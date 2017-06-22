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
        std::ostringstream s(std::ios_base::ate);
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

        s.str("autocmd ");
        s << nvim_->get_augroup()
            << " TabEnter * call rpcnotify("
            << nvim_->get_channel_id()
            << ", 'tabenter', tabpagenr())";
        nvim_->nvim_command(s.str());
        nvim_->signal_tabenter.connect([this](int)
        {
            get_current_tab();
        });

        s.str("autocmd ");
        s << nvim_->get_augroup()
            << " TabClosed,TabNew * call rpcnotify("
            << nvim_->get_channel_id()
            << ", 'tabschanged')";
        nvim_->nvim_command(s.str());
        nvim_->signal_tabschanged.connect([this]()
        {
            list_tabs();
        });

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
            [bh](const BufferInfo &b)->bool { return b.handle == bh; });
    if (match == buffers_.end())
    {
        buffers_.emplace_back(bh);
        auto last = std::prev(buffers_.end());
        auto rqset = RequestSet::create([last, handle](RequestSet *)
        {
            /*
            g_debug("Got info about new buffer %d: %s", handle,
                    last->name.c_str());
            */

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
                        [handle](const BufferInfo &b)->bool
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

std::vector<TabInfo>::iterator BufsAndTabs::get_current_tab_info()
{
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [this](const TabInfo &t)->bool
        {
            return t.handle == current_tab_;
        });
    if (it == tabs_.end())
    {
        g_critical("Current tab handle not in gnvim's list");
    }
    return it;
}

void BufsAndTabs::on_tabs_listed(const msgpack::object &o)
{
    bool know_current = false;
    const msgpack::object_array &ar = o.via.array;
    tabs_ = std::vector<TabInfo>(ar.size);
    for (std::size_t n = 0; n < ar.size; ++n)
    {
        ar.ptr[n].convert(tabs_[n].handle);
        if (tabs_[n].handle == current_tab_)
            know_current = true;
    }
    sig_tabs_listed_.emit(tabs_);

    // Whatever triggered the tab listing should be followed by TabEnter, but
    // because we have async I/O we'll get a signal for that before getting
    // here where we process the list of tabs, so we have to cache the handle
    // from TabEnter and emit our tab_enter signal here.
    if (emitted_current_tab_ != current_tab_)
    {
        if (know_current)
        {
            emitted_current_tab_ = current_tab_;
            sig_tab_enter_.emit(current_tab_);
        }
        else
        {
            get_current_tab();
        }
    }
}

void BufsAndTabs::get_current_tab()
{
    auto prom = MsgpackPromise::create();
    prom->value_signal().connect([this](const msgpack::object &o)
    {
        o.convert(current_tab_);
        if (emitted_current_tab_ != current_tab_)
        {
            if (std::find_if(tabs_.begin(), tabs_.end(),
                [this](const TabInfo &i)->bool
                {
                    return current_tab_ == i.handle;
                }) != tabs_.end())
            {
                emitted_current_tab_ = current_tab_;
                sig_tab_enter_.emit(current_tab_);
            }
        }
    });
    prom->error_signal().connect([this](const msgpack::object &o)
    {
        std::ostringstream s;
        s << o;
        g_critical("Error reading current tab: %s", s.str().c_str());
        current_tab_ = tabs_.begin()->handle;
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

    update_tab_labels_for_buf(handle);
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

void BufsAndTabs::update_tab_labels_for_buf(const VimBuffer &buf)
{
    // for each tab list its windows, get the buf for each win, and check
    // whether it == buf. If tab contains buf its label is updated
    for (auto &tabi: tabs_)
    {
        auto &tab = tabi.handle;
        auto wprom = MsgpackPromise::create();
        // Note we have to use copies, not references, in the lambdas, because
        // they won't be called until after this function exits. Additionally
        // tab is a loop variant.
        wprom->value_signal().connect(
        [this, buf, tab](const msgpack::object &o)
        {
            auto &ar = o.via.array;
            // Get buf info for each win
            for (guint32 n = 0; n < ar.size; ++n)
            {
                VimWindow win;
                ar.ptr[n].convert(win);
                auto bprom = MsgpackPromise::create();
                bprom->value_signal().connect(
                [this, buf, tab, win](const msgpack::object &o)
                {
                    VimBuffer buffer;
                    o.convert(buffer);
                    if (buffer == buf)
                    {
                        get_tab_title(tab, [this, tab](const std::string &s)
                        {
                            sig_tab_label_changed_.emit(tab, s);
                        });
                    }
                });
                nvim_->nvim_win_get_buf(win, bprom);
            }
        });
        nvim_->nvim_tabpage_list_wins(tab, wprom);
    }
}

void BufsAndTabs::get_tab_title(const VimTabpage &tab,
            std::function<void(const std::string &)> final_prom)
{
    // Arrgh, all these nested promises/lambdas,
    // I wish I'd used blocking requests with threaded I/O
    struct TabTitleInfo {
        int count = 0;
        bool modified = false;
        std::string win_name = "???";
    };
    auto tti = std::make_shared<TabTitleInfo>();
    auto rqset = RequestSet::create(
    [tti, final_prom](RequestSet *)
    {
        std::ostringstream s;
        if (tti->count > 1)
            s << tti->count;
        if (tti->modified)
            s << '+';
        if (tti->count > 1 || tti->modified)
            s << ' ';
        s << Glib::path_get_basename(tti->win_name);
        final_prom(s.str());
    });

    auto err = [](const msgpack::object &o)
    {
        std::ostringstream s;
        s << o;
        g_critical("Error getting tab title: %s", s.str().c_str());
    };

    // Captures the result of each nvim_buf_get_modified
    auto mod_capture = [tti](const msgpack::object &o)
    {
        bool mod;
        o.convert(mod);
        if (mod)
            tti->modified = true;
    };

    // For each buffer listed, queries whether it's modified
    auto buf_capture = [this, rqset, mod_capture, err](const msgpack::object &o)
    {
        VimBuffer buf;
        o.convert(buf);
        auto prom = MsgpackPromise::create();
        prom->value_signal().connect(mod_capture);
        prom->error_signal().connect(err);
        nvim_->nvim_buf_get_modified(buf, rqset->get_proxied_promise(prom));
    };

    // Gets a list of the tab's windows, gets the buffer for each one, and then
    // whether it's modified
    try {
        auto prom = MsgpackPromise::create();
        prom->value_signal().connect(
        [this, rqset, tti, buf_capture, err](const msgpack::object &o)
        {
            auto &ar = o.via.array;
            tti->count = ar.size;
            // Get buf info for each win
            for (guint32 n = 0; n < ar.size; ++n)
            {
                VimWindow win;
                ar.ptr[n].convert(win);
                auto prom = MsgpackPromise::create();
                prom->value_signal().connect(buf_capture);
                prom->error_signal().connect(err);
                nvim_->nvim_win_get_buf(win, rqset->get_proxied_promise(prom));
            }
        });
        prom->error_signal().connect(err);
        nvim_->nvim_tabpage_list_wins(tab, rqset->get_proxied_promise(prom));

        // Get tab's current win, its buf and buf's name
        prom = MsgpackPromise::create();
        prom->value_signal().connect(
        [this, rqset, tti, err]
        (const msgpack::object &o) mutable
        {
            VimWindow win;
            o.convert(win);
            auto prom = MsgpackPromise::create();
            prom->value_signal().connect(
            [this, rqset, tti, err](const msgpack::object &o)
            {
                VimBuffer buf;
                o.convert(buf);
                auto prom = MsgpackPromise::create();
                prom->value_signal().connect(
                [tti](const msgpack::object &o)
                {
                    o.convert_if_not_nil(tti->win_name);
                    if (!tti->win_name.size())
                        tti->win_name = _("[No name]");
                });
                prom->error_signal().connect(err);
                nvim_->nvim_buf_get_name(buf, rqset->get_proxied_promise(prom));
            });
            prom->error_signal().connect(err);
            nvim_->nvim_win_get_buf(win, rqset->get_proxied_promise(prom));
        });
        nvim_->nvim_tabpage_get_win(tab, rqset->get_proxied_promise(prom));
        rqset->ready();
    }
    catch (const std::exception &x)
    {
        g_critical("std::exception getting tab title: %s", x.what());
    }
    catch (const Glib::Exception &x)
    {
        g_critical("Glib::Exception getting tab title: %s", x.what().c_str());
    }
}

}
