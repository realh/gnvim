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

// Adapts msg-pack rpc calls into signals with demarshalled arguments. Return
// types aren't supported, it's more straightforward for response handlers to
// send a response directly due to multiple types and error support.

#include "defns.h"

#include <msgpack.hpp>

namespace Gnvim
{

template<class T> struct RemoveConstRef             { typedef T type; };
template<class T> struct RemoveConstRef<const T>    { typedef T type; };
template<class T> struct RemoveConstRef<T &>        { typedef T type; };
template<class T> struct RemoveConstRef<const T &>  { typedef T type; };

class MsgpackAdapterBase {
public:
    MsgpackAdapterBase ()
    {}

    virtual ~MsgpackAdapterBase()
    {}

    // -1 means varargs
    virtual int num_args () = 0;

    void emit (const msgpack::object_array &mp_args);
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) = 0;
};

template<class... T> class MsgpackAdapter: public MsgpackAdapterBase {
public:
    MsgpackAdapter (sigc::signal<void, T...> &sig)
            : MsgpackAdapterBase (), signal_ (sig)
    {}

    virtual int num_args () override;
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override;
private:
    sigc::signal<void, T...> signal_;
};

template<> class MsgpackAdapter<void>: public MsgpackAdapterBase {
public:
    MsgpackAdapter (sigc::signal<void> &sig)
            : MsgpackAdapterBase (), signal_ (sig)
    {}

    virtual int num_args () override { return 0; }
protected:
    virtual void do_emit (const msgpack::object_array &) override
    {
        signal_.emit ();
    }
private:
    sigc::signal<void> signal_;
};

template<> class MsgpackAdapter<const msgpack::object_array &> :
        public MsgpackAdapterBase {
public:
    MsgpackAdapter (sigc::signal<void, const msgpack::object_array &> &sig)
            : MsgpackAdapterBase (), signal_ (sig)
    {}

    virtual int num_args () override { return -1; }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        signal_.emit (mp_args);
    }
private:
    sigc::signal<void, const msgpack::object_array &> signal_;
};

template<class T1> class MsgpackAdapter<T1>: public MsgpackAdapterBase {
public:
    MsgpackAdapter (sigc::signal<void, T1> &sig)
            : MsgpackAdapterBase (), signal_ (sig)
    {}

    virtual int num_args () override { return 1; }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        typename RemoveConstRef<T1>::type a1;
        mp_args.ptr[0].convert (a1);
        signal_.emit (a1);
    }
private:
    sigc::signal<void, T1> signal_;
};

template<class T1, class T2> class MsgpackAdapter<T1, T2>:
        public MsgpackAdapterBase {
public:
    MsgpackAdapter (sigc::signal<void, T1, T2> &sig)
            : MsgpackAdapterBase (), signal_ (sig)
    {}

    virtual int num_args () override { return 2; }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        typename RemoveConstRef<T1>::type a1;
        typename RemoveConstRef<T2>::type a2;
        mp_args.ptr[0].convert (a1);
        mp_args.ptr[1].convert (a2);
        signal_.emit (a1, a2);
    }
private:
    sigc::signal<void, T1, T2> signal_;
};

template<class T1, class T2, class T3> class MsgpackAdapter<T1, T2, T3>:
        public MsgpackAdapterBase {
public:
    MsgpackAdapter (sigc::signal<void, T1, T2, T3> &sig)
            : MsgpackAdapterBase (), signal_ (sig)
    {}

    virtual int num_args () override { return 3; }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        typename RemoveConstRef<T1>::type a1;
        typename RemoveConstRef<T2>::type a2;
        typename RemoveConstRef<T3>::type a3;
        mp_args.ptr[0].convert (a1);
        mp_args.ptr[1].convert (a2);
        mp_args.ptr[2].convert (a3);
        signal_.emit (a1, a2, a3);
    }
private:
    sigc::signal<void, T1, T2, T3> signal_;
};

template<class T1, class T2, class T3, class T4>
class MsgpackAdapter<T1, T2, T3, T4>: public MsgpackAdapterBase {
public:
    MsgpackAdapter (
        sigc::signal<void, T1, T2, T3, T4> &sig)
            : MsgpackAdapterBase (), signal_ (sig)
    {}

    virtual int num_args () override { return 4; }
protected:
    virtual void do_emit (const msgpack::object_array &mp_args) override
    {
        typename RemoveConstRef<T1>::type a1;
        typename RemoveConstRef<T2>::type a2;
        typename RemoveConstRef<T3>::type a3;
        typename RemoveConstRef<T4>::type a4;
        mp_args.ptr[0].convert (a1);
        mp_args.ptr[1].convert (a2);
        mp_args.ptr[2].convert (a3);
        mp_args.ptr[3].convert (a4);
        signal_.emit (a1, a2, a3, a4);
    }
private:
    sigc::signal<void, T1, T2, T3, T4> signal_;
};

}

#endif // GNVIM_MSGPACK_ADAPTER_H
