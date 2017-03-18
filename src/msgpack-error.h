/* msgpack-error.h
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

#ifndef GNVIM_MSGPACK_ERROR_H
#define GNVIM_MSGPACK_ERROR_H

#include "defns.h"

namespace Gnvim
{

class MsgpackError: public Glib::Exception {
public:
    template<class T> MsgpackError (T &&desc) : desc_ (desc)
    {}

    virtual Glib::ustring what () const override;
private:
    Glib::ustring desc_;
};

class MsgpackSendError: public MsgpackError {
public:
    template<class T> MsgpackSendError (T &&desc) : MsgpackError (desc)
    {}
};

class MsgpackResponseError: public MsgpackError {
public:
    template<class T> MsgpackResponseError (T &&desc) : MsgpackError (desc)
    {}
};

class MsgpackArgsError: public MsgpackError {
public:
    template<class T> MsgpackArgsError (T &&desc) : MsgpackError (desc)
    {}
};

}

#endif // GNVIM_MSGPACK_ERROR_H
