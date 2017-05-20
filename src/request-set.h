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

#include <functional>
#include <memory>

#include "msgpack-promise.h"

namespace Gnvim
{

/** Groups a set of MgsgpackPromises so that a function is called when all
 *  the promises have been fulfilled.
 *  It holds a shared_ptr to itself which it clears when the promises are all
 *  fulfilled.
 */
class RequestSet : public std::enable_shared_from_this<RequestSet> {
private:
    using Functor = std::function<void(RequestSet *)>;

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
    /// @param finished Called when all promises have been fulfilled.
    static std::shared_ptr<RequestSet>
    create(Functor finished)
    {
        return (new RequestSet(finished))->shared_from_this();
    }

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
        if (!outstanding_) finished();
    }

    void reset()
    {
        ready_ = false;
        outstanding_ = 0;
    }
private:
    RequestSet(Functor finished) : self_ref_(this), finished_(finished)
    {}

    void finished()
    {
        finished_(this);
        self_ref_.reset();
    }

    void promise_fulfilled()
    {
        if (!--outstanding_ && ready_) finished();
    }

    std::shared_ptr<RequestSet> self_ref_;
    Functor finished_;
    int outstanding_ {0};
    bool ready_ {false};
};


}

#endif // GNVIM_REQUEST_SET_H
