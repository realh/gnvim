/* msgpack-rpc.h
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

#ifndef GNVIM_NVIM_BRIDGE_H
#define GNVIM_NVIM_BRIDGE_H

// Spawns an nvim instance and interfaces with it via msgpack-rpc

#include "defns.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ext-types.h"
#include "msgpack-adapter.h"
#include "msgpack-rpc.h"
#include "start-bundle.h"

namespace Gnvim
{

class NvimBridge {
public:
    NvimBridge();

    ~NvimBridge();

    /// Starts an instance of nvim with --embed.
    void start(const StartBundle &sb);

    /** Starts communication with nvim over an already open pair of streams.
     *  The streams may be pipes from the embedding version of start() or both
     *  refer to the same socket.
     *  @param unified  True if both streams refer to the same r/w stream
     *                  eg a socket.
     */
    void start(RefPtr<Gio::OutputStream> strm_to_nvim,
        RefPtr<Gio::InputStream> strm_from_nvim, bool unified);

    /// Opens a socket for communicating with a remote nvim.
    /// If socket_addr begins with '/' it's assumed to be a Unix socket,
    /// otherwise TCP.
    void start(const std::string &socket_addr);

    /// Connects additional handler to promise to get important info.
    /// Do not reuse the promise for any different request.
    void get_api_info(PromiseHandle promise);

    void start_ui(int width, int height);

    void stop();

    /// @returns true if the nvim instance is owned by us and it's still active.
    bool own_instance()
    {
        return nvim_pid_ != 0;
    }

    void nvim_get_api_info(PromiseHandle promise);

    void nvim_input(const std::string &keys);

    void nvim_command(const std::string &command);

    void nvim_get_option(const std::string &name,
            PromiseHandle promise);

    template<typename T> void nvim_set_option
        (const std::string &name, const T &val)
    {
        rpc_->notify("nvim_set_option", name, val);
    }

    void nvim_get_current_buf(PromiseHandle promise);

    void nvim_list_bufs(PromiseHandle promise);

    void nvim_list_tabs(PromiseHandle promise);

    void nvim_get_current_tab(PromiseHandle promise);

    void nvim_buf_get_name(VimBuffer buf_handle, PromiseHandle promise);

    void nvim_buf_get_modified(VimBuffer buf_handle, PromiseHandle promise);

    void nvim_ui_try_resize(int width, int height);

    void nvim_tabpage_list_wins(VimTabpage tab, PromiseHandle promise);

    void nvim_tabpage_get_win(VimTabpage tab, PromiseHandle promise);

    void nvim_set_current_tabpage(VimTabpage tab);

    void nvim_win_get_buf(VimWindow win, PromiseHandle promise);

    sigc::signal<void, Glib::ustring> &io_error_signal()
    {
        return rpc_->io_error_signal();
    }

    // General notifications
    
    // Second int should actually be bool
    sigc::signal<void, int, int> signal_modified;
    // Here the ints are buffer handles
    sigc::signal<void, int> signal_bufadd;
    sigc::signal<void, int> signal_bufdel;
    sigc::signal<void, int> signal_tabenter;
    sigc::signal<void> signal_tabschanged;      // tab closed or created

    // redraw notifications
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

    guint get_channel_id() const
    {
        return channel_id_;
    }

    /// The group name for autocommands this instance sets up
    const std::string &get_augroup() const
    {
        return augroup_;
    }

    /// Ensures that our augroup is defined in nvim
    void ensure_augroup();
private:
    void map_adapters();

    void on_request(guint32 msgid, std::string method,
            const msgpack::object &args);

    void on_notify(std::string method, const msgpack::object &args);

    void on_api_info_response(const msgpack::object &o);

    using adapter_ptr_t = std::unique_ptr<MsgpackAdapterBase>;
    using map_t = std::map<std::string, adapter_ptr_t>;
    //map_t request_adapters_;
    map_t notify_adapters_;
    map_t redraw_adapters_;

    Glib::Pid nvim_pid_ {0};
    RefPtr<Gio::SocketConnection> socket_;
    RefPtr<MsgpackRpc> rpc_;
    bool ui_attached_ {false};

    guint channel_id_;
    struct {
        int api_compatible, api_level, major, minor, patch;
        bool api_prerelease;
    } version_;
    std::string augroup_;
    bool augroup_defined_ {false};
};

}

#endif // GNVIM_NVIM_BRIDGE_H
