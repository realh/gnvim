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

#ifndef GNVIM_MSGPACK_RPC_H
#define GNVIM_MSGPACK_RPC_H

// Implements msgpack-rpc

#include "defns.h"

#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <giomm.h>

#include <msgpack.hpp>

#include "msgpack-promise.h"

namespace Gnvim
{

// Derived from GObject so we can hold extra references while waiting for idle
// to emit signals. Sending to nvim is all done on main thread, callbacks and
// responses have their own thread
class MsgpackRpc: public Glib::Object {
private:
    using packer_t = msgpack::packer<std::ostringstream>;

    using packer_fn = std::function<void(packer_t &)>;
public:
    constexpr static int REQUEST = 0;
    constexpr static int RESPONSE = 1;
    constexpr static int NOTIFY = 2;

    static RefPtr<MsgpackRpc> create()
    {
        return RefPtr<MsgpackRpc> (new MsgpackRpc());
    }

    void start(int pipe_to_nvim, int pipe_from_nvim);

    virtual ~MsgpackRpc();

    void stop();

    void request(const char *method,
            PromiseHandle promise)
    {
        do_request(method,
                [](packer_t &packer) { packer.pack_array(0); },
                promise);
    }

    template<class... T> void request(const char *method,
            PromiseHandle promise,
            const T &...args)
    {
        do_request(method,
                [args...](packer_t &packer) { pack_args(packer, args...); },
                promise);
    }

    void notify(const char *method)
    {
        do_notify(method,
                [](packer_t &packer) { packer.pack_array(0); });
    }

    template<class... T> void notify(const char *method, T... args)
    {
        do_notify(method,
                [args...](packer_t &packer) { pack_args(packer, args...); });
    }

    template<class T> void response(guint32 msgid, const T &result)
    {
        do_response(msgid,
                [](packer_t &packer) { packer.pack_nil(); },
                [result](packer_t &packer) { packer.pack(result); });
    }

    template<class T> void response_error(guint32 msgid, const T &err)
    {
        do_response(msgid,
                [err](packer_t &packer) { packer.pack(err); },
                [](packer_t &packer) { packer.pack_nil(); });
    }

    sigc::signal<void, guint32, std::string, const msgpack::object &> &
    request_signal()
    {
        return request_signal_;
    }

    sigc::signal<void, std::string, const msgpack::object &> &notify_signal()
    {
        return notify_signal_;
    }

    // This signal is generally caused by the nvim instance quitting
    sigc::signal<void, Glib::ustring> &io_error_signal()
    {
        return io_error_signal_;
    }

    static gint64 ext_to_int(const msgpack::object &o);
protected:
    MsgpackRpc();
private:
    void do_request(const char *method, packer_fn arg_packer,
            PromiseHandle promise);

    void do_notify(const char *method, packer_fn arg_packer);

    void do_response(guint32 msgid,
            packer_fn val_packer, packer_fn err_packer);

    template<class T1> static void
    pack_args(packer_t &packer, const T1 &a1)
    {
        packer.pack_array(1);
        packer.pack(a1);
    }

    template<class T1, class T2> static void
    pack_args(packer_t &packer, const T1 &a1, const T2 &a2)
    {
        packer.pack_array(2);
        packer.pack(a1);
        packer.pack(a2);
    }

    template<class T1, class T2, class T3> static void
    pack_args(packer_t &packer, const T1 &a1, const T2 &a2,
            const T3 &a3)
    {
        packer.pack_array(3);
        packer.pack(a1);
        packer.pack(a2);
        packer.pack(a3);
    }

    template<class T1, class T2, class T3, class T4> static void
    pack_args(packer_t &packer, const T1 &a1, const T2 &a2,
            const T3 &a3, const T4 &a4)
    {
        packer.pack_array(4);
        packer.pack(a1);
        packer.pack(a2);
        packer.pack(a3);
        packer.pack(a4);
    }

    template<class T> static void pack_response(packer_t &packer, const T &a)
    {
        packer.pack(a);
    }

    void send(std::string &&s);

    void start_async_read();

    void async_read(RefPtr<Gio::AsyncResult> &result);

    bool object_received(const msgpack::object &msg);

    bool object_error(char *raw_msg);

    bool dispatch_request(const msgpack::object &msg);

    bool dispatch_response(const msgpack::object &msg);

    bool dispatch_notify(const msgpack::object &msg);

    RefPtr<Gio::OutputStream> strm_to_nvim_;
    RefPtr<Gio::InputStream> strm_from_nvim_;

    bool stop_ {false};
    Gio::SlotAsyncReady async_read_slot_;
    msgpack::unpacker unpacker_;

    std::map<guint32, PromiseHandle> response_promises_;

    sigc::signal<void, guint32, std::string, const msgpack::object &>
            request_signal_;
    sigc::signal<void, std::string, const msgpack::object &> notify_signal_;
    sigc::signal<void, Glib::ustring> io_error_signal_;
    msgpack::object *response_ {nullptr};
    msgpack::object *response_error_ {nullptr};
    RefPtr<Gio::Cancellable> rcv_cancellable_ { Gio::Cancellable::create() };

    constexpr static gsize BUFLEN = 100;

    static guint32 msgid_;
};

class MsgpackDecodeError : public std::exception {
public:
    MsgpackDecodeError(const std::string &msg) : msg_(msg)
    {}

    virtual const char *what() const noexcept override
    {
        return msg_.c_str();
    }
private:
    std::string msg_;

};

}

#endif // GNVIM_MSGPACK_RPC_H
