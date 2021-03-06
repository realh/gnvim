/* nvim-bridge.cpp
 *
 * Copyright(C) 2017 Tony Houghton
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
#include "ext-types.h"

namespace Gnvim
{

NvimBridge::NvimBridge()
{
    map_adapters();
}

/*
static void modify_env(std::vector<std::string> &env,
        const std::string &name, const std::string &value,
        bool overwrite = true)
{
    auto l = name.size();
    auto it = std::find_if(env.begin(), env.end(),
            [l, name](const std::string &a)
    {
        return a.compare(0, l, name) == 0;
    });
    if (it == env.end())
    {
        env.push_back(name + '=' + value);
    }
    else if (overwrite)
    {
        *it = name + '=' + value;
    }
}
*/

void NvimBridge::start(const StartBundle &sb)
{
    int to_nvim_stdin, from_nvim_stdout;
    Glib::spawn_async_with_pipes(sb.get_cwd(),
            sb.get_command_line(), sb.get_environ(),
            Glib::SPAWN_SEARCH_PATH,
            Glib::SlotSpawnChildSetup(), &nvim_pid_,
            &to_nvim_stdin, &from_nvim_stdout);

    start(RefPtr<Gio::OutputStream>
            (Gio::UnixOutputStream::create(to_nvim_stdin, TRUE)),
        RefPtr<Gio::InputStream>
            (Gio::UnixInputStream::create(from_nvim_stdout, TRUE)),
        false);
}

void NvimBridge::start(RefPtr<Gio::OutputStream> strm_to_nvim,
    RefPtr<Gio::InputStream> strm_from_nvim, bool unified)
{
    rpc_ = MsgpackRpc::create();
    rpc_->notify_signal().connect(
            sigc::mem_fun(*this, &NvimBridge::on_notify));
    rpc_->request_signal().connect(
            sigc::mem_fun(*this, &NvimBridge::on_request));

    rpc_->start(strm_to_nvim, strm_from_nvim, unified);
}

void NvimBridge::start(const std::string &addr)
{
    auto client = Gio::SocketClient::create();
    if (addr[0] == '/')
    {
        auto sock_addr = Gio::UnixSocketAddress::create(addr,
                Gio::UNIX_SOCKET_ADDRESS_PATH);
        socket_ = client->connect(sock_addr);
    }
    else
    {
        socket_ = client->connect_to_host(addr, 6666);
    }
    start(socket_->get_output_stream(), socket_->get_input_stream(), true);
}

NvimBridge::~NvimBridge()
{
    stop();
    if (nvim_pid_)
    {
        Glib::spawn_close_pid(nvim_pid_);
        nvim_pid_ = 0;
    }
}

void NvimBridge::map_adapters()
{
    redraw_adapters_.emplace("resize",
        new MsgpackAdapter<int, int> (redraw_resize));
    redraw_adapters_.emplace("clear",
        new MsgpackAdapter<void> (redraw_clear));
    redraw_adapters_.emplace("eol_clear",
        new MsgpackAdapter<void> (redraw_eol_clear));
    redraw_adapters_.emplace("cursor_goto",
        new MsgpackAdapter<int, int> (redraw_cursor_goto));
    redraw_adapters_.emplace("update_fg",
        new MsgpackAdapter<int> (redraw_update_fg));
    redraw_adapters_.emplace("update_bg",
        new MsgpackAdapter<int> (redraw_update_bg));
    redraw_adapters_.emplace("update_sp",
        new MsgpackAdapter<int> (redraw_update_sp));
    redraw_adapters_.emplace("highlight_set",
        new MsgpackAdapter<const msgpack::object &> (redraw_highlight_set));
    redraw_adapters_.emplace("put",
        new MsgpackAdapter<const msgpack::object_array &> (redraw_put));
    redraw_adapters_.emplace("set_scroll_region",
        new MsgpackAdapter<int, int, int, int> (redraw_set_scroll_region));
    redraw_adapters_.emplace("scroll",
        new MsgpackAdapter<int> (redraw_scroll));
    redraw_adapters_.emplace("set_title",
        new MsgpackAdapter<const std::string &> (redraw_set_title));
    redraw_adapters_.emplace("set_icon",
        new MsgpackAdapter<const std::string &> (redraw_set_icon));
    redraw_adapters_.emplace("bell",
        new MsgpackAdapter<void> (redraw_bell));
    redraw_adapters_.emplace("visual_bell",
        new MsgpackAdapter<void> (redraw_visual_bell));
    redraw_adapters_.emplace("mode_change",
        new MsgpackAdapter<const std::string &> (redraw_mode_change));
    /*
    redraw_adapters_.emplace("mouse_on",
        new MsgpackAdapter<void> (redraw_mouse_on));
    redraw_adapters_.emplace("mouse_off",
        new MsgpackAdapter<void> (redraw_mouse_off));
    redraw_adapters_.emplace("busy_on",
        new MsgpackAdapter<void> (redraw_busy_on));
    redraw_adapters_.emplace("busy_off",
        new MsgpackAdapter<void> (redraw_busy_off));
    redraw_adapters_.emplace("suspend",
        new MsgpackAdapter<void> (redraw_suspend));
    redraw_adapters_.emplace("update_menu",
        new MsgpackAdapter<void> (redraw_update_menu));
    redraw_adapters_.emplace("popupmenu_show",
        new MsgpackAdapter<const msgpack::object &, int, int, int>
        (redraw_popupmenu_show));
    redraw_adapters_.emplace("popupmenu_select",
        new MsgpackAdapter<int> (redraw_popupmenu_select));
    redraw_adapters_.emplace("popupmenu_hide",
        new MsgpackAdapter<void> (redraw_popupmenu_hide));
    */

    notify_adapters_.emplace("modified",
        new MsgpackAdapter<int, int> (signal_modified));
    notify_adapters_.emplace("bufadd",
        new MsgpackAdapter<int> (signal_bufadd));
    notify_adapters_.emplace("bufdel",
        new MsgpackAdapter<int> (signal_bufdel));
    notify_adapters_.emplace("tabenter",
        new MsgpackAdapter<int> (signal_tabenter));
    notify_adapters_.emplace("tabenter",
        new MsgpackAdapter<int> (signal_tabenter));
    notify_adapters_.emplace("tabschanged",
        new MsgpackAdapter<void> (signal_tabschanged));
}

void NvimBridge::start_ui(int width, int height)
{
    std::map<std::string, bool> options;
    // Don't actually need to set any options
    //g_debug("nvim-bridge attaching ui");
    rpc_->notify("nvim_ui_attach", width, height, options);
    ui_attached_ = true;
}

void NvimBridge::stop()
{
    if (rpc_)
    {
        if (ui_attached_)
        {
            nvim_command(std::string("autocmd! " + get_augroup()));
            rpc_->notify("nvim_ui_detach");
            if (own_instance())
                rpc_->notify("nvim_command", "qa!");
            ui_attached_ = false;
        }
        rpc_->stop();
    }
}

void NvimBridge::get_api_info(PromiseHandle promise)
{
    promise->value_signal().connect
        (sigc::mem_fun(*this, &NvimBridge::on_api_info_response));
    nvim_get_api_info(promise);
}

void NvimBridge::nvim_get_api_info(PromiseHandle promise)
{
    rpc_->request("nvim_get_api_info", promise);
}

void NvimBridge::nvim_input(const std::string &keys)
{
    //g_debug("nvim_input(%s)", keys.c_str());
    rpc_->notify("nvim_input", keys);
}

void NvimBridge::nvim_command(const std::string &command)
{
    rpc_->notify("nvim_command", command);
}

void NvimBridge::nvim_get_option(const std::string &name,
        PromiseHandle promise)
{
    rpc_->request("nvim_get_option", promise, name);
}

void NvimBridge::nvim_get_current_buf(PromiseHandle promise)
{
    rpc_->request("nvim_get_current_buf", promise);
}

void NvimBridge::nvim_list_bufs(PromiseHandle promise)
{
    rpc_->request("nvim_list_bufs", promise);
}

void NvimBridge::nvim_list_tabs(PromiseHandle promise)
{
    rpc_->request("nvim_list_tabpages", promise);
}

void NvimBridge::nvim_get_current_tab(PromiseHandle promise)
{
    rpc_->request("nvim_get_current_tabpage", promise);
}

void NvimBridge::nvim_buf_get_name(VimBuffer buf_handle,
        PromiseHandle promise)
{
    rpc_->request("nvim_buf_get_name", promise, buf_handle);
}

void NvimBridge::nvim_buf_get_modified(VimBuffer buf_handle,
        PromiseHandle promise)
{
    rpc_->request("nvim_buf_get_option", promise, buf_handle, "modified");
}

void NvimBridge::nvim_ui_try_resize(int width, int height)
{
    rpc_->notify("nvim_ui_try_resize", width, height);
}

void NvimBridge::nvim_tabpage_list_wins(VimTabpage tab, PromiseHandle promise)
{
    rpc_->request("nvim_tabpage_list_wins", promise, tab);
}

void NvimBridge::nvim_tabpage_get_win(VimTabpage tab, PromiseHandle promise)
{
    rpc_->request("nvim_tabpage_get_win", promise, tab);
}

void NvimBridge::nvim_set_current_tabpage(VimTabpage tab)
{
    rpc_->notify("nvim_set_current_tabpage", tab);
}

void NvimBridge::nvim_win_get_buf(VimWindow win, PromiseHandle promise)
{
    rpc_->request("nvim_win_get_buf", promise, win);
}

void NvimBridge::on_request(guint32 msgid, std::string method,
        const msgpack::object &args)
{
    std::ostringstream s;
    s << "nvim sent request " << msgid
            << " '" << method << "' (" << args << ")";
    g_debug("%s", s.str().c_str());
}

void NvimBridge::on_notify(std::string method,
        const msgpack::object &args)
{
    if (method != "redraw")
    {
        const auto &it = notify_adapters_.find(method);
        if (it != notify_adapters_.end())
        {
            it->second->emit(args.via.array);
        }
        else
        {
            std::ostringstream s;
            s << "nvim sent notification '" << method << "' (" << args << ")";
            g_debug("%s", s.str().c_str());
        }
        return;
    }
    //g_debug("nvim-bridge starting redraw");
    redraw_start.emit();
    const msgpack::object_array &ar = args.via.array;
    for (guint32 i = 0; i < ar.size; ++i)
    {
        const auto &method_ar = ar.ptr[i].via.array;
        std::string method_name;
        method_ar.ptr[0].convert(method_name);
        const auto &it = redraw_adapters_.find(method_name);
        if (it != redraw_adapters_.end())
        {
            const auto &emitter = it->second;
            try {
                // put is weird, encoded as["put", [char], [char], ...]
                // (where each char is encoded as a utf-8 str)
                // instead of["put", [str]].
                if (emitter->num_args() == 0 || method_name == "put")
                {
                    emitter->emit(method_ar);
                }
                // highlight_set is also weird, may be sent as
                // ["highlight_set", [{}], [{useful}]] in addition to
                // expected["highlight_set", [{useful}]]
                else if (method_name == "highlight_set"
                        && method_ar.size > 2)
                {
                    emitter->emit(method_ar.ptr[2].via.array);
                }
                else
                {
                    emitter->emit(method_ar.ptr[1].via.array);
                }
            }
            catch(std::exception &e)
            {
                g_critical("std::exception emitting redraw method '%s': %s",
                    method_name.c_str(), e.what());
            }
            catch(Glib::Exception &e)
            {
                g_critical("Glib::Exception emitting redraw method '%s': %s",
                    method_name.c_str(), e.what().c_str());
            }
        }
        //else
        //{
        //    g_debug("Unknown redraw method %s", method_name.c_str());
        //}
    }
    redraw_end.emit();
    //g_debug("nvim-bridge completed redraw");
}

void NvimBridge::on_api_info_response(const msgpack::object &o)
{
    if (o.type != msgpack::type::ARRAY)
    {
        throw MsgpackDecodeError
            ("Response from nvim_get_api_info is not an ARRAY");
    }
    const auto &ar = o.via.array;
    if (ar.size != 2)
    {
        throw MsgpackDecodeError
            ("ARRAY from nvim_get_api_info should have 2 members");
    }
    ar.ptr[0].convert(channel_id_);
    std::ostringstream s;
    s << "gnvim" << channel_id_;
    augroup_ = s.str();

    if (ar.ptr[1].type != msgpack::type::MAP)
    {
        throw MsgpackDecodeError
            ("2nd member of nvim_get_api_info response should be a MAP");
    }

    bool got_types = false, got_version = false;

    const auto &om = ar.ptr[1].via.map;
    gsize i;
    for (i = 0; i < om.size; ++i)
    {
        const auto &kv = om.ptr[i];
        std::string keyname;
        kv.key.convert(keyname);
        if (keyname == "types")
        {
            // If we've got this far it's vanishingly unlikely that we'll
            // encounter errors here
            const auto &tm = kv.val.via.map;
            for (gsize j = 0; j < tm.size; ++j)
            {
                const auto &tkv = tm.ptr[j];
                tkv.key.convert(keyname);
                int *ptc = nullptr;
                if (keyname == "Buffer")
                    ptc = &VimBuffer::type_code;
                else if (keyname == "Tabpage")
                    ptc = &VimTabpage::type_code;
                else if (keyname == "Window")
                    ptc = &VimWindow::type_code;
                if (ptc)
                {
                    const auto &type_info = tkv.val.via.map;
                    for (gsize k = 0; k < type_info.size; ++k)
                    {
                        const auto &field = type_info.ptr[k];
                        field.key.convert(keyname);
                        if (keyname == "id")
                        {
                            field.val.convert(*ptc);
                            break;
                        }
                    }
                }
            }
            got_types = true;
            if (got_version)
                break;
        }
        else if (keyname == "version")
        {
            const auto &vm = kv.val.via.map;
            for (gsize j = 0; j < vm.size; ++j)
            {
                const auto &vkv = vm.ptr[j];
                vkv.key.convert(keyname);
                if (keyname == "api_compatible")
                    vkv.val.convert(version_.api_compatible);
                else if (keyname == "api_level")
                    vkv.val.convert(version_.api_level);
                else if (keyname == "major")
                    vkv.val.convert(version_.major);
                else if (keyname == "minor")
                    vkv.val.convert(version_.minor);
                else if (keyname == "patch")
                    vkv.val.convert(version_.patch);
                else if (keyname == "api_prerelease")
                    vkv.val.convert(version_.api_prerelease);
            }
            got_version = true;
            if (got_types)
                break;
        }
    }
    if (i == om.size)
    {
        throw MsgpackDecodeError
            ("No 'types' field in nvim_get_api_info response");
    }
}

void NvimBridge::ensure_augroup()
{
    if (!augroup_defined_)
    {
        std::ostringstream s;
        s << "augroup " << get_augroup() << "|autocmd!|augroup END";
        nvim_command(s.str());
        augroup_defined_ = true;
    }
}

}
