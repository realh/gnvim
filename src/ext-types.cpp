/* ext-types.cpp
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

#include "defns.h"

#include "ext-types.h"
#include "msgpack-rpc.h"

namespace Gnvim
{

VimExtType &VimExtType::operator=(const msgpack::object &o)
{
    const auto &ext = o.via.ext;
    bytes_ = std::vector<gint8>(ext.data(), ext.data() + ext.size);
    return *this;
}

VimBuffer &VimBuffer::operator=(const msgpack::object &o)
{
    if (o.type != msgpack::type::EXT)
    {
        throw MsgpackDecodeError("Trying to convert non-EXT to VimBuffer");
    }
    if (o.via.ext.type() != type_code)
    {
        throw MsgpackDecodeError
        ("Trying to convert wrong EXT type to VimBuffer");
    }
    VimExtType::operator=(o);
    return *this;
}

int VimBuffer::type_code = 0;

VimTabpage &VimTabpage::operator=(const msgpack::object &o)
{
    if (o.type != msgpack::type::EXT)
    {
        throw MsgpackDecodeError("Trying to convert non-EXT to VimTabpage");
    }
    if (o.via.ext.type() != type_code)
    {
        throw MsgpackDecodeError
        ("Trying to convert wrong EXT type to VimTabpage");
    }
    VimExtType::operator=(o);
    return *this;
}

int VimTabpage::type_code = 0;

VimWindow &VimWindow::operator=(const msgpack::object &o)
{
    if (o.type != msgpack::type::EXT)
    {
        throw MsgpackDecodeError("Trying to convert non-EXT to VimWindow");
    }
    if (o.via.ext.type() != type_code)
    {
        throw MsgpackDecodeError
        ("Trying to convert wrong EXT type to VimWindow");
    }
    VimExtType::operator=(o);
    return *this;
}

int VimWindow::type_code = 0;

} // namespace gnvim
