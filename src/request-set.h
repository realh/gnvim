/* request-set.h
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

#ifndef GNVIM_REQUEST_SET_H
#define GNVIM_REQUEST_SET_H

#include "defns.h"
#include "msgpack-promise.h"

namespace Gnvim
{

template<class T> class RequestSet;

/// Abstract base class for RequestSet.
class RequestSetBase {
private:
    friend class ProxiedPromise;
    class ProxiedPromise : public MsgpackPromise {
    public:
        static std::shared_ptr<MsgpackPromise> create
            (RequestSetBase &rset, std::shared_ptr<MsgpackPromise> promise)
        {
            return std::static_pointer_cast<MsgpackPromise>
                (std::shared_ptr<ProxiedPromise>
                    (new ProxiedPromise(rset, promise)));
        }
    protected:
        ProxiedPromise(RequestSetBase &rset,
                std::shared_ptr<MsgpackPromise> promise);

        void on_value(const msgpack::object &value);

        void on_error(const msgpack::object &value);
    private:
        RequestSetBase &rset_;
        std::shared_ptr<MsgpackPromise> promise_;
    };
public:
    /// Enables creation of an mplementation with automatic type deduction.
    template<class T> static RequestSetBase *create(T finished)
    {
        return new RequestSet<T> (finished);
    }

    virtual ~RequestSetBase() = default;

    /** Creates a proxy for a given promise.
     *  You should connect to the original promise to get the response to the
     *  request, but pass the result of this function to MsgpackRpc::request().
     */
    std::shared_ptr<MsgpackPromise> get_proxied_promise
        (std::shared_ptr<MsgpackPromise> promise)
    {
        ++outstanding_;
        return ProxiedPromise::create(*this, promise);
    }

    /** Call after all promises have been used in requests.
     *  The finished function will not be called until this has been called.
     */
    void ready()
    {
        ready_ = true;
        if(!outstanding_) emit_finished();
    }

    void reset()
    {
        ready_ = false;
        outstanding_ = 0;
    }
protected:
    virtual void emit_finished() = 0;
private:
    void promise_fulfilled()
    {
        if(!--outstanding_ && ready_) emit_finished();
    }

    int outstanding_ {0};
    bool ready_ {false};
};

/** Groups a set of requests, calling a master callback when all promises
 *  have been fulfilled.
 *  @tparam T A void(*)(void) callable type.
 */
template<class T> class RequestSet : public RequestSetBase {
public:
    RequestSet(T finished) : finished_(finished)
    {}
protected:
    virtual void emit_finished() override
    {
        finished_();
    }
private:
    T finished_;
};

}

#endif // GNVIM_REQUEST_SET_H
