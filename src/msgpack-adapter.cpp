/* msgpack-adapter.cpp
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

#include "defns.h"

#include "msgpack-adapter.h"

namespace Gnvim
{

void MsgpackAdapterBase::emit (const msgpack::object_array &mp_args)
{
    int na =  num_args ();
    int nmpa = (int) mp_args.size;
    // When emitting a method with no args, mp_args is the entire notification,
    // including the method name. It may have an empty array as its second
    // member, so its size should be 1 or 2
    if (nmpa != na && (na > 0 || (na == 0 && nmpa > 2)))
    {
        g_critical ("Wrong number of args in msgpack callback; "
                "expected %d, got %d", num_args (), mp_args.size);
        return;
    }
    do_emit (mp_args);
}

}
