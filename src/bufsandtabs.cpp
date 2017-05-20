/* bufsandtabs.cpp
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
#include <iterator>
#include <sstream>

#include "bufsandtabs.h"
#include "nvim-bridge.h"
#include "request-set.h"

namespace Gnvim
{

BufsAndTabs::BufsAndTabs(std::shared_ptr<NvimBridge> nvim) : nvim_(nvim)
{
    nvim_->signal_modified.connect(sigc::mem_fun
            (*this, &BufsAndTabs::on_modified_changed));
}

void BufsAndTabs::start()
{
    get_all_info();
}

void BufsAndTabs::get_all_info()
{
    got_buf_info_ = false;
    conn_got_all_bufs_ = sig_bufs_listed_.connect
    ([this](const std::vector<BufferInfo> &)
    {
        conn_got_all_bufs_.disconnect();
        got_buf_info_ = true;
        if (got_tab_info_)
            got_all_info();
    });
    list_buffers();

    got_tab_info_ = false;
    conn_got_all_tabs_ = sig_tabs_listed_.connect
    ([this](const std::vector<TabInfo> &)
    {
        conn_got_all_tabs_.disconnect();
        got_tab_info_ = true;
        if (got_buf_info_)
            got_all_info();
    });
    list_tabs();
}

void BufsAndTabs::got_all_info()
{
    if (!au_on_)
    {
        nvim_->ensure_augroup();
        std::ostringstream s;
        s << "autocmd " << nvim_->get_augroup()
            << " TextChanged,TextChangedI,BufReadPost,"
                "FileReadPost,FileWritePost * "
            << "if !exists('b:old_mod') || b:old_mod != &modified"
            << "|let b:old_mod = &modified"
            << "|call rpcnotify("
            << nvim_->get_channel_id()
            << ", 'modified', str2nr(expand('<abuf>')), &modified)"
            << "|endif";
        nvim_->nvim_command(s.str());

        s.str("autocmd ");
        s << nvim_->get_augroup()
            << " BufAdd * call rpcnotify("
            << nvim_->get_channel_id()
            << ", 'bufadd', str2nr(expand('<abuf>')))";
        nvim_->nvim_command(s.str());

        s.str("autocmd ");
        s << nvim_->get_augroup()
            << " BufDelete * call rpcnotify("
            << nvim_->get_channel_id()
            << ", 'bufdel', str2nr(expand('<abuf>')))";
        nvim_->nvim_command(s.str());

        au_on_ = true;
    }

    sig_got_all_.emit();
}

void BufsAndTabs::list_buffers()
{
    auto prom = MsgpackPromise::create();
    prom->value_signal().connect
    (sigc::mem_fun(*this, &BufsAndTabs::on_bufs_listed));
    prom->error_signal().connect
    (sigc::mem_fun(*this, &BufsAndTabs::on_bufs_list_error));
    nvim_->nvim_list_bufs(prom);
}

void BufsAndTabs::list_tabs()
{
    auto prom = MsgpackPromise::create();
    prom->value_signal().connect
    (sigc::mem_fun(*this, &BufsAndTabs::on_tabs_listed));
    prom->error_signal().connect
    (sigc::mem_fun(*this, &BufsAndTabs::on_tabs_list_error));
    nvim_->nvim_list_tabs(prom);
}

std::vector<BufferInfo>::iterator BufsAndTabs::add_buffer(int handle)
{
    VimBuffer bh(handle);

    const auto &match = std::find_if(buffers_.begin(), buffers_.end(),
            [bh](const BufferInfo &b) { return b.handle == bh; });
    if (match == buffers_.end())
    {
        buffers_.emplace_back(bh);
        auto last = std::prev(buffers_.end());
        auto rqset = RequestSet::create([last, handle](RequestSet *)
        {
            g_debug("Got info about new buffer %d: %s", handle,
                    last->name.c_str());

        });
        get_buf_info(*last, *rqset, [handle](const msgpack::object &o)
        {
            std::ostringstream s;
            s << o;
            g_critical("Error reading info about new buffer %d: %s",
                    handle, s.str().c_str());
        });
        return last;
    }
    else
    {
        g_warning("add_buffer: handle %d is already in the list", handle);
        return match;
    }
}

void BufsAndTabs::on_bufs_listed(const msgpack::object &o)
{
    const msgpack::object_array &ar = o.via.array;
    struct BufListing {
        std::vector<BufferInfo> bufs;
        bool error;
    };
    auto buf_info = std::make_shared<BufListing>();
    buf_info->bufs = std::vector<BufferInfo>(ar.size);
    buf_info->error = false;

    for (std::size_t n = 0; n < ar.size; ++n)
    {
        ar.ptr[n].convert(buf_info->bufs[n].handle);
        buf_info->bufs[n].modified = false;
    }

    auto rqset = RequestSet::create([this, buf_info](RequestSet *)
    {
        if (buf_info->error)
        {
            sig_bufs_listed_.emit(buf_info->bufs);
        }
        else
        {
            buffers_ = std::move(buf_info->bufs);
            std::sort(buffers_.begin(), buffers_.end());
            auto prom = MsgpackPromise::create();
            prom->value_signal().connect([this](const msgpack::object &o)
            {
                VimBuffer handle(o);
                current_buffer_ = std::find_if(buffers_.begin(), buffers_.end(),
                        [handle](const BufferInfo &b)
                        {
                            return b.handle == handle;
                        });
                sig_bufs_listed_.emit(buffers_);
            });
            prom->error_signal().connect([this](const msgpack::object &o)
            {
                std::ostringstream s;
                s << o;
                g_critical("Error reading current buffer: %s", s.str().c_str());
                current_buffer_ = buffers_.begin();
                sig_bufs_listed_.emit(buffers_);
            });
            nvim_->nvim_get_current_buf(prom);
        }
    });

    auto error_lambda = [buf_info](const msgpack::object &o)
    {
        std::ostringstream s;
        s << o;
        g_critical("Error getting buffer info: %s", s.str().c_str());
        buf_info->error = true;
    };

    for (std::size_t n = 0; n < ar.size; ++n)
    {
        get_buf_info(buf_info->bufs[n], *rqset, error_lambda);
    }

    rqset->ready();
}

void BufsAndTabs::get_buf_info(BufferInfo &binfo, RequestSet &rqset,
        sigc::slot<void, const msgpack::object &> on_err)
{
    auto prom = MsgpackPromise::create();
    prom->value_signal().connect([binfo](const msgpack::object &o) mutable
    {
        o.convert(binfo.name);
    });
    prom->error_signal().connect(on_err);
    nvim_->nvim_buf_get_name(binfo.handle,
            rqset.get_proxied_promise(prom));

    prom = MsgpackPromise::create();
    prom->value_signal().connect([binfo](const msgpack::object &o) mutable
    {
        o.convert(binfo.modified);
    });
    prom->error_signal().connect(on_err);
    nvim_->nvim_buf_get_modified(binfo.handle,
            rqset.get_proxied_promise(prom));
}

void BufsAndTabs::on_bufs_list_error(const msgpack::object &o)
{
    std::ostringstream s;
    s << o;
    g_critical("Error listing buffers: %s", s.str().c_str());
    std::vector<BufferInfo> nobufs;
    sig_bufs_listed_.emit(nobufs);
}

void BufsAndTabs::on_tabs_listed(const msgpack::object &o)
{
    const msgpack::object_array &ar = o.via.array;
    tabs_ = std::vector<TabInfo>(ar.size);
    for (std::size_t n = 0; n < ar.size; ++n)
    {
        ar.ptr[n].convert(tabs_[n].handle);
    }

    auto prom = MsgpackPromise::create();
    prom->value_signal().connect([this](const msgpack::object &o)
    {
        VimTabpage handle(o);
        current_tab_ = std::find_if(tabs_.begin(), tabs_.end(),
            [handle](const TabInfo &t)
            {
                return t.handle == handle;
            });
        sig_tabs_listed_.emit(tabs_);
    });
    prom->error_signal().connect([this](const msgpack::object &o)
    {
        std::ostringstream s;
        s << o;
        g_critical("Error reading current tab: %s", s.str().c_str());
        current_tab_ = tabs_.begin();
        sig_tabs_listed_.emit(tabs_);
    });
    nvim_->nvim_get_current_tab(prom);
}

void BufsAndTabs::on_tabs_list_error(const msgpack::object &o)
{
    std::ostringstream s;
    s << o;
    g_critical("Error listing tabs: %s", s.str().c_str());
    std::vector<TabInfo> notabs;
    sig_tabs_listed_.emit(notabs);
}

void BufsAndTabs::on_modified_changed(int handle, bool modified)
{
    for (auto &buf: buffers_)
    {
        if (buf.handle == VimBuffer(handle))
        {
            buf.modified = modified;
            break;
        }
    }
}

bool BufsAndTabs::any_modified() const
{
    for (const auto &buf: buffers_)
    {
        if (buf.modified)
            return true;
    }
    return false;
}

}
