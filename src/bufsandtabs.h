/* bufsandtabs.h
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

#ifndef GNVIM_BUFS_AND_TABS_H
#define GNVIM_BUFS_AND_TABS_H

#include "defns.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ext-types.h"
#include "msgpack-promise.h"
#include "request-set.h"

namespace Gnvim
{

struct BufferInfo {
    VimBuffer handle;
    std::string name;
    bool modified;

    BufferInfo() : handle(0), modified(false) {}

    BufferInfo(const VimBuffer &h, const std::string &n = "", bool mod = false)
        : handle(h), name(n), modified(mod)
    {}

    bool operator<(const BufferInfo &b)
    {
        return handle < b.handle;
    }
};

struct TabInfo {
    VimTabpage handle;

    TabInfo() : handle(0) {}

    TabInfo(const VimTabpage &t) : handle(t) {}

    bool operator<(const TabInfo &t)
    {
        return handle < t.handle;
    }
};
    

/// Keeps track of the buffers and tabs managed by each nvim instance.
class BufsAndTabs {
public:
    BufsAndTabs(std::shared_ptr<NvimBridge> nvim);

    /// Sets up internal listeners and calls get_all_info().
    void start();

    /// Gets the complete lists of buffers and tabs.
    void get_all_info();

    /// Raised when all the requests from get_all_info have been answered.
    sigc::signal<void> &signal_got_all_info()
    {
        return sig_got_all_;
    }

    // Gets buffer info from nvim and emits on_bufs_listed.
    void list_buffers();

    // Gets tab info from nvim and emits on_tabs_listed.
    void list_tabs();

    /// @returns true if any buffers are modified.
    bool any_modified() const;

    /// Raised when all the requests from list_buffers have been answered.
    /// If there was an error the parameter to the signal is the partially
    /// received list and the main copy is unchanged.
    sigc::signal<void, const std::vector<BufferInfo> &> &
    signal_buffers_listed()
    {
        return sig_bufs_listed_;
    }

    sigc::signal<void, const std::vector<TabInfo> &> &
    signal_tabs_listed()
    {
        return sig_tabs_listed_;
    }

    sigc::signal<void, int> &signal_tab_enter()
    {
        return sig_tab_enter_;
    }

    const std::vector<BufferInfo> &get_buffers() const
    {
        return buffers_;
    }

    const std::vector<BufferInfo>::iterator &get_current_buffer()
    {
        return current_buffer_;
    }

    const std::vector<TabInfo> &get_tabs() const
    {
        return tabs_;
    }

    const VimTabpage &get_current_tab_handle()
    {
        return current_tab_;
    }

    std::vector<TabInfo>::iterator get_current_tab_info();

    /// Call when notified of bufadd.
    /// @returns iterator to the new buffer in the vector
    std::vector<BufferInfo>::iterator add_buffer(int handle);

    /// Call when notified of bufdel
    void del_buffer(int handle);

    /// Works out the text that should appear in a tab label by checking the
    /// vim-windows it contains.
    void get_tab_title(const VimTabpage &tab,
            std::function<void(const std::string &)> prom);
private:
    void on_bufs_listed(const msgpack::object &o);
    void on_bufs_list_error(const msgpack::object &o);

    void on_tabs_listed(const msgpack::object &o);
    void on_tabs_list_error(const msgpack::object &o);

    void got_all_info();

    void on_modified_changed(int buf, bool modified);

    void get_buf_info(BufferInfo &binfo, RequestSet &rqset,
            sigc::slot<void, const msgpack::object &> on_err);

    void get_current_tab();

    std::shared_ptr<NvimBridge> nvim_;

    std::vector<BufferInfo> buffers_;
    std::vector<BufferInfo>::iterator current_buffer_;

    std::vector<TabInfo> tabs_;
    VimTabpage current_tab_;
    VimTabpage emitted_current_tab_;

    sigc::signal<void> sig_got_all_;
    sigc::connection conn_got_all_tabs_, conn_got_all_bufs_;
    sigc::signal<void, const std::vector<BufferInfo> &> sig_bufs_listed_;
    sigc::signal<void, const std::vector<TabInfo> &> sig_tabs_listed_;
    sigc::signal<void, int> sig_tab_enter_;

    bool au_on_ {false};
    bool got_buf_info_, got_tab_info_;
};

}

#endif // GNVIM_BUFS_AND_TABS_H
