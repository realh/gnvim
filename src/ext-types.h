/* ext-types.h
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

#ifndef GNVIM_EXT_TYPES_H
#define GNVIM_EXT_TYPES_H

#include "defns.h"

#include <vector>

#include <msgpack.hpp>

namespace Gnvim
{

/// Base class for nvim's Buffer, Window and Tabpage handles which are packed
/// in msgpack as EXT.
class VimExtType {
protected:
    VimExtType() = default;
    VimExtType(const VimExtType &vet) = default;
    VimExtType(VimExtType &&vet) = default;
    VimExtType &operator=(const VimExtType &vet) = default;
    VimExtType &operator=(VimExtType &&vet) = default;

    VimExtType &operator=(const msgpack::object &o);

    bool operator==(const VimExtType &vet) const
    {
        return bytes_ == vet.bytes_;
    }

    template<class S> void pack_body(msgpack::packer<S> &packer)
    {
        packer.pack_ext_body(bytes_.data(), bytes_.size());
    }

    std::vector<gint8> bytes_;
};

class VimBuffer : public VimExtType {
public:
    VimBuffer(const msgpack::object &o)
    {
        *this = o;
    }

    VimBuffer &operator=(const msgpack::object &o);

    VimBuffer() = default;
    VimBuffer(const VimBuffer &vb) = default;
    VimBuffer(VimBuffer &&vb) = default;
    VimBuffer &operator=(const VimBuffer &vb) = default;
    VimBuffer &operator=(VimBuffer &&vb) = default;

    bool operator==(const VimBuffer &vb) const
    {
        return VimExtType::operator==(vb);
    }

    template<class S> void pack(msgpack::packer<S> &packer)
    {
        packer.pack_ext(bytes_.size(), type_code);
        pack_body(packer);
    }

    /// Must be set from nvim_get_api_info before use.
    static gint8 type_code;
};

class VimTabpage : public VimExtType {
public:
    VimTabpage(const msgpack::object &o)
    {
        *this = o;
    }

    VimTabpage &operator=(const msgpack::object &o);

    VimTabpage() = default;
    VimTabpage(const VimTabpage &vt) = default;
    VimTabpage(VimTabpage &&vt) = default;
    VimTabpage &operator=(const VimTabpage &vt) = default;
    VimTabpage &operator=(VimTabpage &&vt) = default;

    bool operator==(const VimTabpage &vt) const
    {
        return VimExtType::operator==(vt);
    }

    template<class S> void pack(msgpack::packer<S> &packer)
    {
        packer.pack_ext(bytes_.size(), type_code);
        pack_body(packer);
    }

    /// Must be set from nvim_get_api_info before use.
    static gint8 type_code;
};

class VimWindow : public VimExtType {
public:
    VimWindow(const msgpack::object &o)
    {
        *this = o;
    }

    VimWindow &operator=(const msgpack::object &o);

    VimWindow() = default;
    VimWindow(const VimWindow &vw) = default;
    VimWindow(VimWindow &&vw) = default;
    VimWindow &operator=(const VimWindow &vw) = default;
    VimWindow &operator=(VimWindow &&vw) = default;

    bool operator==(const VimWindow &vw) const
    {
        return VimExtType::operator==(vw);
    }

    template<class S> void pack(msgpack::packer<S> &packer)
    {
        packer.pack_ext(bytes_.size(), type_code);
        pack_body(packer);
    }

    /// Must be set from nvim_get_api_info before use.
    static gint8 type_code;
};

} // namespace gnvim

namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {

using namespace Gnvim;

template<> struct convert<VimBuffer> {
    msgpack::object const& operator()
    (msgpack::object const &o, VimBuffer &vb) const
    {
        vb = o;
        return o;
    }
};

template<> struct convert<VimTabpage> {
    msgpack::object const& operator()
    (msgpack::object const &o, VimTabpage &vt) const
    {
        vt = o;
        return o;
    }
};

template<> struct convert<VimWindow> {
    msgpack::object const& operator()
    (msgpack::object const &o, VimWindow &vw) const
    {
        vw = o;
        return o;
    }
};

template <> struct pack<VimBuffer> {
    template <class S> msgpack::packer<S>& operator()
    (msgpack::packer<S> &packer, VimBuffer const &vb) const
    {
        vb.pack(packer);
        return packer;
    }
};

} // namespace adaptor
} // namespace API_VERSION...
} // namespace msgpack


#endif // GNVIM_EXT_TYPES_H
