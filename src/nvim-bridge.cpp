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
        new MsgpackAdapter<int, int> (nvim_resize, true));
    redraw_adapters_.emplace ("clear",
        new MsgpackAdapter<void> (nvim_clear, true));
    redraw_adapters_.emplace ("eol_clear",
        new MsgpackAdapter<void> (nvim_eol_clear, true));
    redraw_adapters_.emplace ("cursor_goto",
        new MsgpackAdapter<int, int> (nvim_cursor_goto, true));
    redraw_adapters_.emplace ("update_fg",
        new MsgpackAdapter<int> (nvim_update_fg, true));
    redraw_adapters_.emplace ("update_bg",
        new MsgpackAdapter<int> (nvim_update_bg, true));
    redraw_adapters_.emplace ("update_sp",
        new MsgpackAdapter<int> (nvim_update_sp, true));
    redraw_adapters_.emplace ("highlight_set",
        new MsgpackAdapter<const msgpack::object &> (nvim_highlight_set, true));
    redraw_adapters_.emplace ("put",
        new MsgpackAdapter<const std::string &> (nvim_put, true));
    redraw_adapters_.emplace ("set_scroll_region",
        new MsgpackAdapter<int, int, int, int> (nvim_set_scroll_region, true));
    redraw_adapters_.emplace ("scroll",
        new MsgpackAdapter<int> (nvim_scroll, true));
    redraw_adapters_.emplace ("set_title",
        new MsgpackAdapter<const std::string &> (nvim_set_title, true));
    redraw_adapters_.emplace ("set_icon",
        new MsgpackAdapter<const std::string &> (nvim_set_icon, true));
    redraw_adapters_.emplace ("mouse_on",
        new MsgpackAdapter<void> (nvim_mouse_on, true));
    redraw_adapters_.emplace ("mouse_off",
        new MsgpackAdapter<void> (nvim_mouse_off, true));
    redraw_adapters_.emplace ("busy_on",
        new MsgpackAdapter<void> (nvim_busy_on, true));
    redraw_adapters_.emplace ("busy_off",
        new MsgpackAdapter<void> (nvim_busy_off, true));
    redraw_adapters_.emplace ("suspend",
        new MsgpackAdapter<void> (nvim_suspend, true));
    redraw_adapters_.emplace ("bell",
        new MsgpackAdapter<void> (nvim_bell, true));
    redraw_adapters_.emplace ("visual_bell",
        new MsgpackAdapter<void> (nvim_visual_bell, true));
    redraw_adapters_.emplace ("update_menu",
        new MsgpackAdapter<void> (nvim_update_menu, true));
    redraw_adapters_.emplace ("mode_change",
        new MsgpackAdapter<const std::string &> (nvim_mode_change, true));
    redraw_adapters_.emplace ("popupmenu_show",
        new MsgpackAdapter<const msgpack::object &, int, int, int> (nvim_popupmenu_show, true));
    redraw_adapters_.emplace ("popupmenu_select",
        new MsgpackAdapter<int> (nvim_popupmenu_select, true));
    redraw_adapters_.emplace ("popupmenu_hide",
        new MsgpackAdapter<void> (nvim_popupmenu_hide, true));
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
        msgpack::object args)
{
    if (method == "redraw")
    std::cout << "nvim sent request " << msgid << " '" << method << "' ("
            << args << ")" << std::endl;
}

void NvimBridge::on_notify (std::string method, msgpack::object args)
{
    std::cout << "nvim sent notification '" << method << "' (" << args << ")"
            << std::endl;
}

void NvimBridge::on_error (Glib::ustring desc)
{
    std::cerr << "Error communicating with nvim: " << desc << std::endl;
    error_signal_.emit (desc);
}

}
