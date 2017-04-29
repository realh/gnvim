/* msgpack-promise.h
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

#ifndef GNVIM_MSGPACK_PROMISE_H
#define GNVIM_MSGPACK_PROMISE_H

// Blocking the main thread to wait for a response risks causing deadlocks
// if nvim is also waiting for a response from us, so we'll use a simple
// promise/future system

#include "defns.h"

#include <memory>

#include <msgpack.hpp>

namespace Gnvim
{

class MsgpackPromise {
public:
    static std::shared_ptr<MsgpackPromise> create()
    {
        // Can't use make_shared here because constructor is protected
        return std::shared_ptr<MsgpackPromise> (new MsgpackPromise());
    }

    MsgpackPromise(const MsgpackPromise &) = delete;
    MsgpackPromise &operator=(const MsgpackPromise &) = delete;

    virtual ~MsgpackPromise() = default;

    sigc::signal<void, const msgpack::object &> &value_signal()
    {
        return value_sig_;
    }

    sigc::signal<void, const msgpack::object &> &error_signal()
    {
        return error_sig_;
    }

    void emit_value(const msgpack::object &value)
    {
        value_sig_.emit(value);
    }

    void emit_error(const msgpack::object &error)
    {
        error_sig_.emit(error);
    }
protected:
    MsgpackPromise() = default;
private:
    sigc::signal<void, const msgpack::object &> value_sig_;
    sigc::signal<void, const msgpack::object &> error_sig_;
};

}

#endif // GNVIM_MSGPACK_PROMISE_H
