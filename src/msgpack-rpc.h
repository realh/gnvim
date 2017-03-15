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

#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

#include <giomm.h>

#include <msgpack.hpp>

namespace Gnvim
{

class MsgpackRpc {
public:
    constexpr static int REQUEST = 0;
    constexpr static int RESPONSE = 1;
    constexpr static int NOTIFY = 2;

    MsgpackRpc (int pipe_to_nvim, int pipe_from_nvim);

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
    }

    template<class T> void response_error (guint32 msgid, const T &result)
    {
        std::ostringstream s;
        packer_t packer (s);
        create_response (packer, msgid);
        packer.pack (result);
        packer.pack_nil ();
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

    template<class T> void wait_for_response (guint32 msgid, T &response);

    RefPtr<Gio::OutputStream> strm_to_nvim_;
    RefPtr<Gio::InputStream> strm_from_nvim_;

    static guint32 msgid_;
};

}

#endif // GNVIM_MSGPACK_RPC_H
