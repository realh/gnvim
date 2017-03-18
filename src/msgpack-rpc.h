/* msgpack-rpc.h
 *
 * Copyright (C) 2017 Tony Houghton
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

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include <giomm.h>

#include <msgpack.hpp>

#include "msgpack-error.h"

namespace Gnvim
{

// Derived from GObject so we can hold extra references while waiting for idle
// to emit signals. Sending to nvim is all done on main thread, callbacks and
// responses have their own thread
class MsgpackRpc: public Glib::Object {
public:
    constexpr static int REQUEST = 0;
    constexpr static int RESPONSE = 1;
    constexpr static int NOTIFY = 2;

    using Error = MsgpackError;
    using SendError = MsgpackSendError;
    using ResponseError = MsgpackResponseError;

    static RefPtr<MsgpackRpc> create (int pipe_to_nvim, int pipe_from_nvim)
    {
        return RefPtr<MsgpackRpc> (
                new MsgpackRpc (pipe_to_nvim, pipe_from_nvim));
    }

    virtual ~MsgpackRpc ();

    // Signals may still be raised after calling stop () because they're called
    // from idle handlers. Although sigc is not generally thread-safe,
    // the connect_once () methods are, so it's better here than
    // Glib::Dispatcher because our signals have arguments.
    void stop ();

    template<class R> R request (const char *method)
    {
        std::ostringstream s;
        packer_t packer (s);
        auto msgid = create_request (packer, method);
        packer.pack_array (0);
        send (s.str());
        R result;
        wait_for_response (msgid, result);
        return result;
    }

    template<class R, class... T> R request (const char *method, T... args)
    {
        std::ostringstream s;
        packer_t packer (s);
        auto msgid = create_request (packer, method);
        ArgArray<std::ostringstream> aa (args...);
        aa.pack (packer);
        send (s.str());
        R result;
        wait_for_response (msgid, result);
        return result;
    }

    template<class T> void response (guint32 msgid, const T &result)
    {
        std::ostringstream s;
        packer_t packer (s);
        create_response (packer, msgid);
        packer.pack_nil ();
        packer.pack (result);
        send (s.str());
    }

    template<class T> void response_error (guint32 msgid, const T &result)
    {
        std::ostringstream s;
        packer_t packer (s);
        create_response (packer, msgid);
        packer.pack (result);
        packer.pack_nil ();
        send (s.str());
    }

    void notify (const char *method)
    {
        std::ostringstream s;
        packer_t packer (s);
        create_notify (packer, method);
        packer.pack_array (0);
        send (s.str());
    }

    template<class... T> void notify (const char *method, T... args)
    {
        std::ostringstream s;
        packer_t packer (s);
        create_notify (packer, method);
        ArgArray<std::ostringstream> aa (args...);
        aa.pack (packer);
        send (s.str());
    }

    sigc::signal<void, guint32, std::string, msgpack::object_array>
            request_signal ()
    {
        return request_signal_;
    }

    sigc::signal<void, std::string, msgpack::object_array> notify_signal ()
    {
        return notify_signal_;
    }

    sigc::signal<void, Glib::ustring> rcv_error_signal ()
    {
        return rcv_error_signal_;
    }
protected:
    MsgpackRpc (int pipe_to_nvim, int pipe_from_nvim);
private:
    template<class S> class PackableBase {
    public:
        virtual void pack(msgpack::packer<S> &) = 0;

        virtual ~PackableBase ()
        {}
    };

    template<class S, class T> class Packable: public PackableBase<S> {
    public:
        Packable (const T &value) : value_ (value)
        {}

        Packable (T &&value) : value_ (value)
        {}

        void pack(msgpack::packer<S> &packer)
        {
            packer.pack (value_);
        }

        MSGPACK_DEFINE (value_);
    private:
        T value_;
    };

    template<class S> class ArgArray {
    public:
        template<class... T> ArgArray (T... args)
        {
            add_args (args...);
        }

        void pack(msgpack::packer<S> &packer)
        {
            packer.pack_array (args_.length());
            for (auto i: args_)
            {
                packer.pack (*i);
            }
        }
    private:
        using ptr_t = std::unique_ptr<PackableBase<S>>;

        template<class T> void add_args (T arg)
        {
            args_.push_back (ptr_t (new Packable<S, T> (arg)));
        }

        template<class T, class... V> void add_args (T arg, V... args)
        {
            add_args (arg);
            add_args (args...);
        }

        std::vector<ptr_t> args_;
    };

    using packer_t = msgpack::packer<std::ostringstream>;

    guint32 create_request (packer_t &packer, const char *method);

    void create_response (packer_t &packer, guint32 msgid);

    void create_notify (packer_t &packer, const char *method);

    void create_message (packer_t &packer, int type);

    void send (std::string &&s);

    void run_rcv_thread ();

    bool object_received (const msgpack::object &);

    bool object_error (char *raw_msg);

    template<class T> void wait_for_response (guint32 msgid, T &response)
    {
        msgpack::object *ro = wait_for_response (msgid);
        ro->convert (response);
        delete ro;
    }

    msgpack::object *wait_for_response (guint32 msgid);

    bool dispatch_request (const msgpack::object_array &msg);

    bool dispatch_response (const msgpack::object_array &msg);

    bool dispatch_notify (const msgpack::object_array &msg);

    RefPtr<Gio::OutputStream> strm_to_nvim_;
    RefPtr<Gio::InputStream> strm_from_nvim_;

    std::atomic_bool stop_ {false};

    std::atomic<guint32> response_msgid_ {0};
    std::mutex response_mutex_;
    std::condition_variable response_cond_;
    std::thread rcv_thread_;
    sigc::signal<void, guint32, std::string, msgpack::object_array>
            request_signal_;
    sigc::signal<void, std::string, msgpack::object_array> notify_signal_;
    sigc::signal<void, Glib::ustring> rcv_error_signal_;
    msgpack::object *response_ {nullptr};
    msgpack::object *response_error_ {nullptr};
    RefPtr<Gio::Cancellable> rcv_cancellable_ { Gio::Cancellable::create () };

    constexpr static gsize BUFLEN = 100;

    static guint32 msgid_;
};

}

#endif // GNVIM_MSGPACK_RPC_H
