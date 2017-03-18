/* adapter.h
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

#ifndef GNVIM_ADAPTER_H
#define GNVIM_ADAPTER_H

// Adapts msg-pack rpc calls into signals with arguments. Return types
// aren't supported, it's more straightforward for response handlers
// to send a response directly due to multiple types and error support.

#include "defns.h"

#include <msgpack.hpp>

namespace Gnvim
{

class AdapterBase {
public:
    virtual ~AdapterBase()
    {}

    bool check_args (const msgpack::object_array &mp_args)
    {
        return mp_args.size == num_args ();
    }

    virtual guint32 num_args () = 0;

    virtual void emit (const msgpack::object_array &mp_args) = 0;
};

class Adapter0: public AdapterBase {
public:
    Adapter0 (sigc::signal<void> &signal) : signal_ (signal)
    {}

    virtual guint32 num_args () override
    {
        return 0;
    }

    virtual void emit (const msgpack::object_array &) override
    {
        signal_.emit ();
    }
private:
    sigc::signal<void> &signal_;
};

template<class T1> class Adapter1: public AdapterBase {
public:
    Adapter1 (sigc::signal<void, T1> &signal) : signal_ (signal)
    {}

    virtual guint32 num_args () override
    {
        return 1;
    }

    virtual void emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        mp_args.ptr[0].convert (a1);
        signal_.emit (a1);
    }
private:
    sigc::signal<void, T1> signal_;
};

template<class T1, class T2> class Adapter2: public AdapterBase {
public:
    Adapter2 (sigc::signal<void, T1, T2> &signal) : signal_ (signal)
    {}

    virtual guint32 num_args () override
    {
        return 2;
    }

    virtual void emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        T2 a2;
        mp_args.ptr[0].convert (a1);
        mp_args.ptr[1].convert (a2);
        signal_.emit (a1, a2);
    }
private:
    sigc::signal<void, T1, T2> signal_;
};

template<class T1, class T2, class T3> class Adapter3: public AdapterBase {
public:
    Adapter3 (sigc::signal<void, T1, T2, T3> &signal) : signal_ (signal)
    {}

    virtual guint32 num_args () override
    {
        return 3;
    }

    virtual void emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        T2 a2;
        T3 a3;
        mp_args.ptr[0].convert (a1);
        mp_args.ptr[1].convert (a2);
        mp_args.ptr[2].convert (a3);
        signal_.emit (a1, a2, a3);
    }
private:
    sigc::signal<void, T1, T2, T3> signal_;
};

template<class T1, class T2, class T3, class T4> class Adapter4
        : public AdapterBase
{
public:
    Adapter4 (sigc::signal<void, T1, T2, T3, T4> &signal) : signal_ (signal)
    {}

    virtual guint32 num_args () override
    {
        return 4;
    }

    virtual void emit (const msgpack::object_array &mp_args) override
    {
        T1 a1;
        T2 a2;
        T3 a3;
        T4 a4;
        mp_args.ptr[0].convert (a1);
        mp_args.ptr[1].convert (a2);
        mp_args.ptr[2].convert (a3);
        mp_args.ptr[3].convert (a4);
        signal_.emit (a1, a2, a3, a4);
    }
private:
    sigc::signal<void, T1, T2, T3, T4> signal_;
};

class Adapter {
public:
    Adapter (sigc::signal<void> &signal)
            : adapter_ (new Adapter0 (signal))
    {}

    ~Adapter ()
    {
        delete adapter_;
    }

    void emit (const msgpack::object_array &mp_args);
private:
    AdapterBase *adapter_;
};

}

#endif // GNVIM_ADAPTER_H
