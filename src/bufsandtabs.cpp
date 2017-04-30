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
#include <sstream>

#include "bufsandtabs.h"
#include "nvim-bridge.h"
#include "request-set.h"

namespace Gnvim
{

BufsAndTabs::BufsAndTabs(NvimBridge &nvim) : nvim_ (nvim)
{}

void BufsAndTabs::start()
{
    get_all_info();
}

void BufsAndTabs::get_all_info()
{
    conn_got_all_ = sig_bufs_listed_.connect
    ([this](const std::vector<BufferInfo> &)
    {
        conn_got_all_.disconnect();
        sig_got_all_.emit();
    });
    list_buffers();
}

void BufsAndTabs::list_buffers()
{
    auto prom = MsgpackPromise::create();
    prom->value_signal().connect
    (sigc::mem_fun(*this, &BufsAndTabs::on_bufs_listed));
    prom->error_signal().connect
    (sigc::mem_fun(*this, &BufsAndTabs::on_bufs_list_error));
    nvim_.nvim_list_bufs(prom);
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

    delete rqset_;
    rqset_ = RequestSet::create([this, buf_info](RequestSet *)
    {
        if (buf_info->error)
        {
            sig_bufs_listed_.emit(buf_info->bufs);
        }
        else
        {
            buffers_ = std::move(buf_info->bufs);
            std::sort (buffers_.begin (), buffers_.end ());
            auto prom = MsgpackPromise::create();
            prom->value_signal ().connect ([this](const msgpack::object &o)
            {
                VimBuffer handle(o);
                current_buffer_ = std::find_if(buffers_.begin(), buffers_.end(),
                        [handle](const BufferInfo &b)
                        {
                            return b.handle == handle;
                        });
                sig_bufs_listed_.emit(buffers_);
            });
            prom->error_signal ().connect ([this](const msgpack::object &o)
            {
                std::ostringstream s;
                s << o;
                g_critical("Error reading current buffer: %s", s.str().c_str());
                current_buffer_ = buffers_.begin ();
                sig_bufs_listed_.emit(buffers_);
            });
            nvim_.nvim_get_current_buf(prom);
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
        auto prom = MsgpackPromise::create();
        prom->value_signal().connect([buf_info, n](const msgpack::object &o)
        {
            o.convert(buf_info->bufs[n].name);
        });
        prom->error_signal().connect(error_lambda);
        nvim_.nvim_buf_get_name (buf_info->bufs[n].handle,
                rqset_->get_proxied_promise(prom));

        prom = MsgpackPromise::create();
        prom->value_signal().connect([buf_info, n](const msgpack::object &o)
        {
            o.convert(buf_info->bufs[n].modified);
            g_debug("Buf %ld %s modified", n,
                    buf_info->bufs[n].modified ? "is" : "is not");
        });
        prom->error_signal().connect(error_lambda);
        nvim_.nvim_buf_get_changedtick (buf_info->bufs[n].handle,
                rqset_->get_proxied_promise(prom));
    }

    rqset_->ready();
}

void BufsAndTabs::on_bufs_list_error(const msgpack::object &o)
{
    std::ostringstream s;
    s << o;
    g_critical("Error listing buffers: %s", s.str().c_str());
    std::vector<BufferInfo> nobufs;
    sig_bufs_listed_.emit(nobufs);
}

}
