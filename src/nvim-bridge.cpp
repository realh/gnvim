/* msgpack-rpc.cpp
 *
 * Copyright (C) 2017 Tony Houghton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "defns.h"

#include <iostream>
#include <string>
#include <vector>

#include "nvim-bridge.h"

namespace Gnvim
{

NvimBridge::NvimBridge ()
{
    static std::vector<std::string> argv {"nvim", "--embed"};
    static std::vector<std::string> envp;
    int to_nvim_stdin, from_nvim_stdout;

    map_adapters ();

    Glib::spawn_async_with_pipes ("", argv, envp, Glib::SPAWN_SEARCH_PATH,
            Glib::SlotSpawnChildSetup (), &nvim_pid_,
            &to_nvim_stdin, &from_nvim_stdout);

    rpc_ = MsgpackRpc::create ();
    rpc_->notify_signal ().connect (
            sigc::mem_fun (*this, &NvimBridge::on_notify));
    rpc_->request_signal ().connect (
            sigc::mem_fun (*this, &NvimBridge::on_request));
    rpc_->rcv_error_signal ().connect (
            sigc::mem_fun (*this, &NvimBridge::on_error));
    rpc_->start (to_nvim_stdin, from_nvim_stdout);
}

NvimBridge::~NvimBridge ()
{
    if (nvim_pid_)
    {
        Glib::spawn_close_pid (nvim_pid_);
        nvim_pid_ = 0;
    }
}

void NvimBridge::map_adapters ()
{
    redraw_adapters_.emplace ("resize",
        new MsgpackAdapter<int, int> (nvim_resize));
    redraw_adapters_.emplace ("clear",
        new MsgpackAdapter<void> (nvim_clear));
    redraw_adapters_.emplace ("eol_clear",
        new MsgpackAdapter<void> (nvim_eol_clear));
    redraw_adapters_.emplace ("cursor_goto",
        new MsgpackAdapter<int, int> (nvim_cursor_goto));
    redraw_adapters_.emplace ("update_fg",
        new MsgpackAdapter<int> (nvim_update_fg));
    redraw_adapters_.emplace ("update_bg",
        new MsgpackAdapter<int> (nvim_update_bg));
    redraw_adapters_.emplace ("update_sp",
        new MsgpackAdapter<int> (nvim_update_sp));
    redraw_adapters_.emplace ("highlight_set",
        new MsgpackAdapter<const msgpack::object &> (nvim_highlight_set));
    redraw_adapters_.emplace ("put",
        new MsgpackAdapter<const msgpack::object_array &> (nvim_put));
    redraw_adapters_.emplace ("set_scroll_region",
        new MsgpackAdapter<int, int, int, int> (nvim_set_scroll_region));
    redraw_adapters_.emplace ("scroll",
        new MsgpackAdapter<int> (nvim_scroll));
    redraw_adapters_.emplace ("set_title",
        new MsgpackAdapter<const std::string &> (nvim_set_title));
    redraw_adapters_.emplace ("set_icon",
        new MsgpackAdapter<const std::string &> (nvim_set_icon));
    redraw_adapters_.emplace ("mouse_on",
        new MsgpackAdapter<void> (nvim_mouse_on));
    redraw_adapters_.emplace ("mouse_off",
        new MsgpackAdapter<void> (nvim_mouse_off));
    redraw_adapters_.emplace ("busy_on",
        new MsgpackAdapter<void> (nvim_busy_on));
    redraw_adapters_.emplace ("busy_off",
        new MsgpackAdapter<void> (nvim_busy_off));
    redraw_adapters_.emplace ("suspend",
        new MsgpackAdapter<void> (nvim_suspend));
    redraw_adapters_.emplace ("bell",
        new MsgpackAdapter<void> (nvim_bell));
    redraw_adapters_.emplace ("visual_bell",
        new MsgpackAdapter<void> (nvim_visual_bell));
    redraw_adapters_.emplace ("update_menu",
        new MsgpackAdapter<void> (nvim_update_menu));
    redraw_adapters_.emplace ("mode_change",
        new MsgpackAdapter<const std::string &> (nvim_mode_change));
    redraw_adapters_.emplace ("popupmenu_show",
        new MsgpackAdapter<const msgpack::object &, int, int, int>
        (nvim_popupmenu_show));
    redraw_adapters_.emplace ("popupmenu_select",
        new MsgpackAdapter<int> (nvim_popupmenu_select));
    redraw_adapters_.emplace ("popupmenu_hide",
        new MsgpackAdapter<void> (nvim_popupmenu_hide));
}

void NvimBridge::start_gui (int width, int height)
{
    std::map<std::string, bool> options;
    // Don't actually need to set any options
    rpc_->notify ("nvim_ui_attach", width, height, options);
}

void NvimBridge::stop ()
{
    if (rpc_)
        rpc_->stop ();
}

void NvimBridge::on_request (guint32 msgid, std::string method,
        const msgpack::object &args)
{
    std::cout << "nvim sent request " << msgid
            << " '" << method << "' (" << args << ")" << std::endl;
}

void NvimBridge::on_notify (std::string method,
        const msgpack::object &args)
{
    if (method == "redraw")
    {
        const msgpack::object_array &ar = args.via.array;
        for (guint32 i = 0; i < ar.size; ++i)
        {
            const auto &method_ar = ar.ptr[i].via.array;
            std::string method_name;
            method_ar.ptr[0].convert (method_name);
            const auto &it = redraw_adapters_.find (method_name);
            if (it != redraw_adapters_.end ())
            {
                const auto &emitter = it->second;
                try {
                    // put is weird, encoded as ["put", [char], [char], ...]
                    // (where each char is encoded as a utf-8 str)
                    // instead of ["put", [str]].
                    if (emitter->num_args () == 0 || method_name == "put")
                    {
                        emitter->emit (method_ar);
                    }
                    else
                        emitter->emit (method_ar.ptr[1].via.array);
                }
                catch (std::exception &e)
                {
                    std::cerr << "std::exception emitting redraw method '"
                        << method_name << "' : " << e.what() << std::endl;
                }
                catch (Glib::Exception &e)
                {
                    std::cerr << "Glib::Exception emitting redraw method '"
                        << method_name << "' : " << e.what() << std::endl;
                }
            }
            else
            {
                g_warning ("No adapater for redraw method '%s'",
                        method_name.c_str());
            }
        }
    }
    else
    {
        std::cout << "nvim sent notification '" << method << "' ("
                << args << ")" << std::endl;
    }
}

void NvimBridge::on_error (Glib::ustring desc)
{
    std::cerr << "Error communicating with nvim: " << desc << std::endl;
    error_signal_.emit (desc);
}

}
