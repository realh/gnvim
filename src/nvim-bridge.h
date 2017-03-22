/* msgpack-rpc.h
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

#ifndef GNVIM_NVIM_BRIDGE_H
#define GNVIM_NVIM_BRIDGE_H

// Spawns an nvim instance and interfaces with it via msgpack-rpc

#include "defns.h"

#include <map>
#include <memory>

#include "msgpack-adapter.h"
#include "msgpack-rpc.h"

namespace Gnvim
{

class NvimBridge {
public:
    NvimBridge ();

    ~NvimBridge ();

    void start_gui (int width, int height);

    void stop ();

    sigc::signal<void, Glib::ustring> &error_signal ()
    {
        return error_signal_;
    }

    sigc::signal<void, int, int> nvim_resize;
    sigc::signal<void> nvim_clear;
    sigc::signal<void> nvim_eol_clear;
    sigc::signal<void, int, int> nvim_cursor_goto;
    sigc::signal<void, int> nvim_update_fg;
    sigc::signal<void, int> nvim_update_bg;
    sigc::signal<void, int> nvim_update_sp;
    sigc::signal<void, const msgpack::object &> nvim_highlight_set;
    sigc::signal<void, const std::string &> nvim_put;
    sigc::signal<void, int, int, int, int> nvim_set_scroll_region;
    sigc::signal<void, int> nvim_scroll;
    sigc::signal<void, const std::string &> nvim_set_title;
    sigc::signal<void, const std::string &> nvim_set_icon;
    sigc::signal<void> nvim_mouse_on;
    sigc::signal<void> nvim_mouse_off;
    sigc::signal<void> nvim_busy_on;
    sigc::signal<void> nvim_busy_off;
    sigc::signal<void> nvim_suspend;
    sigc::signal<void> nvim_bell;
    sigc::signal<void> nvim_visual_bell;
    sigc::signal<void> nvim_update_menu;
    sigc::signal<void, const std::string &> nvim_mode_change;
    sigc::signal<void, const msgpack::object &, int, int, int>
            nvim_popupmenu_show;
    sigc::signal<void, int> nvim_popupmenu_select;
    sigc::signal<void> nvim_popupmenu_hide;

private:
    void map_adapters ();

    void on_request (guint32 msgid, std::string method,
            const msgpack::object &args);

    void on_notify (std::string method, const msgpack::object &args);

    void on_error (Glib::ustring);

    using adapter_ptr_t = std::unique_ptr<MsgpackAdapterBase>;
    using map_t = std::map<std::string, adapter_ptr_t>;
    map_t request_adapters_;
    map_t notify_adapters_;
    map_t redraw_adapters_;

    sigc::signal<void, Glib::ustring> error_signal_;

    RefPtr<MsgpackRpc> rpc_;
    Glib::Pid nvim_pid_;
};

}

#endif // GNVIM_NVIM_BRIDGE_H
