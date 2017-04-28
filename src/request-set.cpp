
/* request-set.h
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

#include <sstream>

#include "request-set.h"

namespace Gnvim
{

RequestSetBase::ProxiedPromise::ProxiedPromise (RequestSetBase &rset,
                std::shared_ptr<MsgpackPromise> promise)
    : rset_ (rset), promise_ (promise)
{
    value_signal ().connect
        (sigc::mem_fun (*this, &ProxiedPromise::on_value));
    error_signal ().connect
        (sigc::mem_fun (*this, &ProxiedPromise::on_error));
}

void RequestSetBase::ProxiedPromise::on_value (const msgpack::object &value)
{
    promise_->value_signal ().emit (value);
    rset_.promise_fulfilled ();
}

void RequestSetBase::ProxiedPromise::on_error (const msgpack::object &error)
{
    std::ostringstream s;
    s << error;
    g_debug ("Error response to ProxiedPromise request %s", s.str ().c_str ());
    promise_->error_signal ().emit (error);
    rset_.promise_fulfilled ();
}

}
