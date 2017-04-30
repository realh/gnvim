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

template<class T> class RequestSetConcrete;

/// Abstract base class for RequestSetConcrete.
class RequestSet {
private:
    friend class ProxiedPromise;
    class ProxiedPromise : public MsgpackPromise {
    public:
        static PromiseHandle create
            (RequestSet &rset, PromiseHandle promise)
        {
            return std::static_pointer_cast<MsgpackPromise>
                (std::shared_ptr<ProxiedPromise>
                    (new ProxiedPromise(rset, promise)));
        }
    protected:
        ProxiedPromise(RequestSet &rset,
                PromiseHandle promise);

        void on_value(const msgpack::object &value);

        void on_error(const msgpack::object &value);
    private:
        RequestSet &rset_;
        PromiseHandle promise_;
    };
public:
    /** Enables creation of an implementation with automatic type deduction.
     *  @tparam T a function/functor of type void (*)(RequestSet *rqset);.
     *  @param finished A tempting place to delete the rqset, but it might
     *                  not be safe to do so; defer to something like
     *                  Glib::signal_idle().connect_once() if you need to
     *                  delete this.
     */
    template<class T> static RequestSet *create(T finished)
    {
        return new RequestSetConcrete<T> (finished);
    }

    virtual ~RequestSet() = default;

    /** Creates a proxy for a given promise.
     *  You should connect to the original promise to get the response to the
     *  request, but pass the result of this function to MsgpackRpc::request().
     *  This could have been implemented by just connecting additional handlers
     *  to the original promise, but a proxy offers a better guarantee that
     *  all client promise signals are handled before calling the 'finished'
     *  handler.
     */
    PromiseHandle get_proxied_promise(PromiseHandle promise)
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
        if (!outstanding_) emit_finished();
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
        if (!--outstanding_ && ready_) emit_finished();
    }

    int outstanding_ {0};
    bool ready_ {false};
};

/** Groups a set of requests, calling a master callback when all promises
 *  have been fulfilled.
 *  @tparam T A void(*)(void) callable type.
 */
template<class T> class RequestSetConcrete : public RequestSet {
public:
    RequestSetConcrete(T finished) : finished_(finished)
    {}
protected:
    virtual void emit_finished() override
    {
        finished_(this);
    }
private:
    T finished_;
};

}

#endif // GNVIM_REQUEST_SET_H
