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

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "nvim-bridge.h"

namespace Gnvim
{

NvimBridge::NvimBridge ()
{
    map_adapters ();
}

static void modify_env (std::vector<std::string> &env,
        const std::string &name, const std::string &value,
        bool overwrite = true)
{
    auto l = name.size ();
    auto it = std::find_if (env.begin (), env.end (),
            [l, name] (const std::string &a)
    {
        return a.compare (0, l, name) == 0;
    });
    if (it == env.end ())
    {
        env.push_back (name + '=' + value);
    }
    else if (overwrite)
    {
        *it = name + '=' + value;
    }
}

void NvimBridge::start (RefPtr<Gio::ApplicationCommandLine> cl,
        const std::string &init_file)
{
    std::vector<std::string> args {"nvim"};
    int argc;
    char **argv = cl->get_arguments (argc);
    // Check whether -u or --embed are already present
    bool u = false, embed = false;
    for (int n = 1; n < argc; ++n)
    {
        if (!strcmp (argv[n], "-u"))
            u = true;
        else if (!strcmp (argv[n], "--embed"))
            embed = true;
    }

    if (!embed)
        args.push_back ("--embed");

    // Load custom init file if present and -u wasn't overridden
    if (!u && init_file.size ())
    {
        args.push_back ("-u");
        args.push_back (init_file);
    }

    for (int n = 1; n < argc; ++n)
    {
        args.push_back (argv[n]);
    }

    auto envp = cl->get_environ ();
    modify_env (envp, "XDG_CONFIG_HOME", Glib::get_user_config_dir (), false);
    modify_env (envp, "XDG_DATA_HOME", Glib::get_user_data_dir (), false);

    int to_nvim_stdin, from_nvim_stdout;
    Glib::spawn_async_with_pipes (cl->get_cwd (),
            args, envp, Glib::SPAWN_SEARCH_PATH,
            Glib::SlotSpawnChildSetup (), &nvim_pid_,
            &to_nvim_stdin, &from_nvim_stdout);

    rpc_ = MsgpackRpc::create ();
    rpc_->notify_signal ().connect (
            sigc::mem_fun (*this, &NvimBridge::on_notify));
    rpc_->request_signal ().connect (
            sigc::mem_fun (*this, &NvimBridge::on_request));

    rpc_->start (to_nvim_stdin, from_nvim_stdout);
}

NvimBridge::~NvimBridge ()
{
    stop ();
    if (nvim_pid_)
    {
        Glib::spawn_close_pid (nvim_pid_);
        nvim_pid_ = 0;
    }
}

void NvimBridge::map_adapters ()
{
    redraw_adapters_.emplace ("resize",
        new MsgpackAdapter<int, int> (redraw_resize));
    redraw_adapters_.emplace ("clear",
        new MsgpackAdapter<void> (redraw_clear));
    redraw_adapters_.emplace ("eol_clear",
        new MsgpackAdapter<void> (redraw_eol_clear));
    redraw_adapters_.emplace ("cursor_goto",
        new MsgpackAdapter<int, int> (redraw_cursor_goto));
    redraw_adapters_.emplace ("update_fg",
        new MsgpackAdapter<int> (redraw_update_fg));
    redraw_adapters_.emplace ("update_bg",
        new MsgpackAdapter<int> (redraw_update_bg));
    redraw_adapters_.emplace ("update_sp",
        new MsgpackAdapter<int> (redraw_update_sp));
    redraw_adapters_.emplace ("highlight_set",
        new MsgpackAdapter<const msgpack::object &> (redraw_highlight_set));
    redraw_adapters_.emplace ("put",
        new MsgpackAdapter<const msgpack::object_array &> (redraw_put));
    redraw_adapters_.emplace ("set_scroll_region",
        new MsgpackAdapter<int, int, int, int> (redraw_set_scroll_region));
    redraw_adapters_.emplace ("scroll",
        new MsgpackAdapter<int> (redraw_scroll));
    redraw_adapters_.emplace ("set_title",
        new MsgpackAdapter<const std::string &> (redraw_set_title));
    redraw_adapters_.emplace ("set_icon",
        new MsgpackAdapter<const std::string &> (redraw_set_icon));
    redraw_adapters_.emplace ("bell",
        new MsgpackAdapter<void> (redraw_bell));
    redraw_adapters_.emplace ("visual_bell",
        new MsgpackAdapter<void> (redraw_visual_bell));
    redraw_adapters_.emplace ("mode_change",
        new MsgpackAdapter<const std::string &> (redraw_mode_change));
    /*
    redraw_adapters_.emplace ("mouse_on",
        new MsgpackAdapter<void> (redraw_mouse_on));
    redraw_adapters_.emplace ("mouse_off",
        new MsgpackAdapter<void> (redraw_mouse_off));
    redraw_adapters_.emplace ("busy_on",
        new MsgpackAdapter<void> (redraw_busy_on));
    redraw_adapters_.emplace ("busy_off",
        new MsgpackAdapter<void> (redraw_busy_off));
    redraw_adapters_.emplace ("suspend",
        new MsgpackAdapter<void> (redraw_suspend));
    redraw_adapters_.emplace ("update_menu",
        new MsgpackAdapter<void> (redraw_update_menu));
    redraw_adapters_.emplace ("popupmenu_show",
        new MsgpackAdapter<const msgpack::object &, int, int, int>
        (redraw_popupmenu_show));
    redraw_adapters_.emplace ("popupmenu_select",
        new MsgpackAdapter<int> (redraw_popupmenu_select));
    redraw_adapters_.emplace ("popupmenu_hide",
        new MsgpackAdapter<void> (redraw_popupmenu_hide));
    */
}

void NvimBridge::start_ui (int width, int height)
{
    std::map<std::string, bool> options;
    // Don't actually need to set any options
    rpc_->notify ("nvim_ui_attach", width, height, options);
}

void NvimBridge::stop ()
{
    if (rpc_)
    {
        rpc_->notify ("nvim_ui_detach");
        rpc_->stop ();
    }
}

void NvimBridge::nvim_input (const std::string &keys)
{
    rpc_->notify ("nvim_input", keys);
}

void NvimBridge::nvim_get_option (const std::string &name,
        std::shared_ptr<MsgpackPromise> promise)
{
    rpc_->request ("nvim_get_option", promise, name);
}

void NvimBridge::nvim_ui_try_resize (int width, int height)
{
    rpc_->notify ("nvim_ui_try_resize", width, height);
}


void NvimBridge::on_request (guint32 msgid, std::string method,
        const msgpack::object &args)
{
    std::ostringstream s;
    s << "nvim sent request " << msgid
            << " '" << method << "' (" << args << ")";
    g_debug ("%s", s.str ().c_str ());
}

void NvimBridge::on_notify (std::string method,
        const msgpack::object &args)
{
    if (method != "redraw")
    {
        std::ostringstream s;
        s << "nvim sent notification '" << method << "' (" << args << ")";
        g_debug ("%s", s.str ().c_str ());
    }
    redraw_start.emit ();
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
                // highlight_set is also weird, may be sent as
                // ["highlight_set", [{}], [{useful}]] in addition to
                // expected ["highlight_set", [{useful}]]
                else if (method_name == "highlight_set"
                        && method_ar.size > 2)
                {
                    emitter->emit (method_ar.ptr[2].via.array);
                }
                else
                {
                    emitter->emit (method_ar.ptr[1].via.array);
                }
            }
            catch (std::exception &e)
            {
                g_critical ("std::exception emitting redraw method '%s': %s",
                    method_name.c_str (), e.what());
            }
            catch (Glib::Exception &e)
            {
                g_critical ("Glib::Exception emitting redraw method '%s': %s",
                    method_name.c_str (), e.what().c_str ());
            }
        }
        //else
        //{
        //    g_debug ("Unknown redraw method %s", method_name.c_str ());
        //}
    }
    redraw_end.emit ();
}

}
