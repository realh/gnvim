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