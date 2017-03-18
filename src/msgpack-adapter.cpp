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
#include "msgpack-error.h"

namespace Gnvim
{

void MsgpackAdapter::emit (const msgpack::object_array &mp_args)
{
    if (!adapter_->check_args (mp_args))
    {
        char *s = g_strdup_printf ("Wrong number of args in msgpack callback; "
                "expected %d, got %d", adapter_->num_args (), mp_args.size);
        Glib::ustring us (s);
        g_free (s);
        throw MsgpackArgsError (us);
    }
    adapter_->emit (mp_args);
}

}
