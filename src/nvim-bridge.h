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
#include <string>
#include <vector>

#include "msgpack-adapter.h"
#include "msgpack-rpc.h"

namespace Gnvim
{

class NvimBridge {
public:
    NvimBridge ();

    ~NvimBridge ();

    void start (const RefPtr<Gio::ApplicationCommandLine> cl);

    void start_ui (int width, int height);

    void stop ();

    void nvim_input (const std::string &keys);

    void nvim_get_option (const std::string &name,
            std::shared_ptr<MsgpackPromise> promise);

    sigc::signal<void, Glib::ustring> &io_error_signal ()
    {
        return rpc_->io_error_signal ();
    }

    sigc::signal<void> &ready_signal ()
    {
        return ready_signal_;
    }

    int get_default_width ()
    {
        return default_cols_;
    }

    int get_default_height ()
    {
        return default_lines_;
    }

    sigc::signal<void> redraw_start;
    sigc::signal<void, int, int> redraw_resize;
    sigc::signal<void> redraw_clear;
    sigc::signal<void> redraw_eol_clear;
    sigc::signal<void, int, int> redraw_cursor_goto;
    sigc::signal<void, int> redraw_update_fg;
    sigc::signal<void, int> redraw_update_bg;
    sigc::signal<void, int> redraw_update_sp;
    sigc::signal<void, const msgpack::object &> redraw_highlight_set;
    // Note that first member of array is method name
    sigc::signal<void, const msgpack::object_array &> redraw_put;
    sigc::signal<void, int, int, int, int> redraw_set_scroll_region;
    sigc::signal<void, int> redraw_scroll;
    sigc::signal<void, const std::string &> redraw_set_title;
    sigc::signal<void, const std::string &> redraw_set_icon;
    sigc::signal<void> redraw_bell;
    sigc::signal<void> redraw_visual_bell;
    sigc::signal<void, const std::string &> redraw_mode_change;
    sigc::signal<void> redraw_end;
    // The following are ignored, some may be implemented in future
    /*
    sigc::signal<void> redraw_mouse_on;
    sigc::signal<void> redraw_mouse_off;
    sigc::signal<void> redraw_busy_on;
    sigc::signal<void> redraw_busy_off;
    sigc::signal<void> redraw_suspend;
    sigc::signal<void> redraw_update_menu;
    sigc::signal<void, const msgpack::object &, int, int, int>
            redraw_popupmenu_show;
    sigc::signal<void, int> redraw_popupmenu_select;
    sigc::signal<void> redraw_popupmenu_hide;
    */

private:
    void map_adapters ();

    void on_request (guint32 msgid, std::string method,
            const msgpack::object &args);

    void on_notify (std::string method, const msgpack::object &args);

    using adapter_ptr_t = std::unique_ptr<MsgpackAdapterBase>;
    using map_t = std::map<std::string, adapter_ptr_t>;
    map_t request_adapters_;
    map_t notify_adapters_;
    map_t redraw_adapters_;

    RefPtr<MsgpackRpc> rpc_;
    Glib::Pid nvim_pid_;

    sigc::signal<void> ready_signal_;
    int default_cols_ = 80;
    int default_lines_ = 36;
    std::shared_ptr<MsgpackPromise> cols_promise_, lines_promise_;
    void on_cols_promise_value (const msgpack::object &);
    void on_cols_promise_error (const msgpack::object &);
    void on_lines_promise_value (const msgpack::object &);
    void on_lines_promise_error (const msgpack::object &);

    static std::vector<Glib::ustring> envp_;
};

}

#endif // GNVIM_NVIM_BRIDGE_H
