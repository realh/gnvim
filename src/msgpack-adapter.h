/* msgpack-adapter.h
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

#ifndef GNVIM_MSGPACK_ADAPTER_H
#define GNVIM_MSGPACK_ADAPTER_H

// Adapts msg-pack rpc calls into signals with arguments. Return types
// aren't supported, it's more straightforward for response handlers
// to send a response directly due to multiple types and error support.

#include "defns.h"

#include <msgpack.hpp>

namespace Gnvim
{

class MsgpackAdapterBase {
public:
    MsgpackAdapterBase (bool skip_arg0 = true) : skip_arg0_ (skip_arg0 ? 1 : 0)
    {}

    virtual ~MsgpackAdapterBase()
    {}

    bool check_args (const msgpack::object_array &mp_args)
    {
        return mp_args.size == num_args () + skip_arg0_;
    }

    virtual guint32 num_args () = 0;

    void emit (const msgpack::object_array &mp_args);
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) = 0;

    int skip_arg0_;
};

class MsgpackAdapter0: public MsgpackAdapterBase {
public:
    MsgpackAdapter0 (bool skip_arg0 = true) : MsgpackAdapterBase(skip_arg0)
    {}

    virtual guint32 num_args () override
    {
        return 0;
    }

    sigc::signal<void> &get_signal ()
    {
        return signal_;
    }
protected:
    virtual void do_emit (const msgpack::object_array &) override
    {
        signal_.emit ();
    }
private:
    sigc::signal<void> signal_;
};

template<class T1> class MsgpackAdapter1: public MsgpackAdapterBase {
public:
    MsgpackAdapter1 (bool skip_arg0 = true) : MsgpackAdapterBase(skip_arg0)
    {}

    virtual guint32 num_args () override
    {
        return 1;
    }

    sigc::signal<void, T1> &get_signal ()
    {
        return signal_;
    }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        mp_args.ptr[skip_arg0_].convert (a1);
        signal_.emit (a1);
    }
private:
    sigc::signal<void, T1> signal_;
};

template<class T1, class T2> class MsgpackAdapter2: public MsgpackAdapterBase {
public:
    MsgpackAdapter2 (bool skip_arg0 = true) : MsgpackAdapterBase(skip_arg0)
    {}

    virtual guint32 num_args () override
    {
        return 2;
    }

    sigc::signal<void, T1, T2> &get_signal ()
    {
        return signal_;
    }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        T2 a2;
        mp_args.ptr[skip_arg0_].convert (a1);
        mp_args.ptr[1 + skip_arg0_].convert (a2);
        signal_.emit (a1, a2);
    }
private:
    sigc::signal<void, T1, T2> signal_;
};

template<class T1, class T2, class T3> class MsgpackAdapter3
        : public MsgpackAdapterBase {
public:
    MsgpackAdapter3 (bool skip_arg0 = true) : MsgpackAdapterBase(skip_arg0)
    {}

    virtual guint32 num_args () override
    {
        return 3;
    }

    sigc::signal<void, T1, T2, T3> &get_signal ()
    {
        return signal_;
    }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        T2 a2;
        T3 a3;
        mp_args.ptr[skip_arg0_].convert (a1);
        mp_args.ptr[1 + skip_arg0_].convert (a2);
        mp_args.ptr[2 + skip_arg0_].convert (a3);
        signal_.emit (a1, a2, a3);
    }
private:
    sigc::signal<void, T1, T2, T3> signal_;
};

template<class T1, class T2, class T3, class T4> class MsgpackAdapter4
        : public MsgpackAdapterBase
{
public:
    MsgpackAdapter4 (bool skip_arg0 = true) : MsgpackAdapterBase(skip_arg0)
    {}

    virtual guint32 num_args () override
    {
        return 4;
    }

    sigc::signal<void, T1, T2, T3, T4> &get_signal ()
    {
        return signal_;
    }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        T2 a2;
        T3 a3;
        T4 a4;
        mp_args.ptr[skip_arg0_].convert (a1);
        mp_args.ptr[1 + skip_arg0_].convert (a2);
        mp_args.ptr[2 + skip_arg0_].convert (a3);
        mp_args.ptr[3 + skip_arg0_].convert (a4);
        signal_.emit (a1, a2, a3, a4);
    }
private:
    sigc::signal<void, T1, T2, T3, T4> signal_;
};

}

#endif // GNVIM_MSGPACK_ADAPTER_H
