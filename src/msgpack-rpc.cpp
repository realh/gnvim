/* msgpack-rpc.cpp
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

#include "msgpack-rpc.h"

namespace Gnvim
{

Glib::ustring MsgpackRpc::Error::what () const
{
    return desc_;
}

MsgpackRpc::MsgpackRpc ()
        : async_read_slot_ (sigc::mem_fun (this, &MsgpackRpc::async_read))
{
}

MsgpackRpc::~MsgpackRpc ()
{
    stop ();
}

void MsgpackRpc::start (int pipe_to_nvim, int pipe_from_nvim)
{
    strm_to_nvim_ = Gio::UnixOutputStream::create (pipe_to_nvim, TRUE);
    strm_from_nvim_ = Gio::UnixInputStream::create (pipe_from_nvim, TRUE);
    start_async_read ();
}

void MsgpackRpc::request (const char *method,
            std::shared_ptr<MsgpackPromise> promise)
{
    std::ostringstream s;
    packer_t packer (s);
    auto msgid = create_request (packer, method);
    packer.pack_array (0);
    response_promises_[msgid] = promise;
    send (s.str());
}

void MsgpackRpc::stop ()
{
    stop_ = true;
    rcv_cancellable_->cancel ();
    strm_from_nvim_->close ();
    strm_to_nvim_->close ();
}

guint32 MsgpackRpc::create_request (packer_t &packer, const char *method)
{
    packer.pack_array (4);
    packer.pack_int (REQUEST);
    packer.pack_uint32 (++msgid_);
    packer.pack (method);
    return msgid_;
}

void MsgpackRpc::create_response (packer_t &packer, guint32 msgid)
{
    packer.pack_array (4);
    packer.pack_int (RESPONSE);
    packer.pack_uint32 (msgid);
}

void MsgpackRpc::create_notify (packer_t &packer, const char *method)
{
    packer.pack_array (3);
    packer.pack_int (NOTIFY);
    packer.pack (method);
}

void MsgpackRpc::send (std::string &&s)
{
    gsize nwritten;
    if (strm_to_nvim_->write_all (s, nwritten))
    {
        strm_to_nvim_->flush ();
    }
    else
    {
        nwritten = 0;
    }
    if (!nwritten)
    {
        throw SendError ("No bytes written to nvim");
    }
}

void MsgpackRpc::start_async_read ()
{
    if (stop_)
        return;
    unpacker_.reserve_buffer (BUFLEN);
    reference ();
    strm_from_nvim_->read_async (unpacker_.buffer (), BUFLEN,
            async_read_slot_, rcv_cancellable_);
}

void MsgpackRpc::async_read (RefPtr<Gio::AsyncResult> &result)
{
    if (!stop_)
    {
        auto nread = strm_from_nvim_->read_finish (result);
        if (nread < 0)
        {
            stop ();
            rcv_error_signal_.emit (_("I/O error reading from nvim"));
            return;
        }
        else
        {
            unpacker_.buffer_consumed (nread);
            msgpack::unpacked unpacked;
            while (!stop_ && unpacker_.next (unpacked))
            {
                object_received (unpacked.get ());
            }
            start_async_read ();
        }
    }
    unreference ();
}

bool MsgpackRpc::object_error (char *raw_msg)
{
    stop_ = true;
    auto msg = Glib::ustring (raw_msg);
    g_free (raw_msg);
    rcv_error_signal ().emit (msg);
    return false;
}

bool MsgpackRpc::object_received (const msgpack::object &mob)
{
    if (mob.type != msgpack::type::ARRAY)
    {
        return object_error (g_strdup_printf (
                    _("msgpack expected ARRAY, received type %d"), mob.type));
    }

    const msgpack::object_array &array = mob.via.array;
    msgpack::object &msg_type_o = array.ptr[0];
    if (msg_type_o.type != msgpack::type::POSITIVE_INTEGER)
    {
        return object_error (g_strdup_printf (
                    _("msgpack expected type field POSITIVE_INTEGER, "
                        "received type %d"), msg_type_o.type));
    }

    int type = msg_type_o.via.i64;
    switch (type)
    {
        case REQUEST:
            //g_debug ("Received request");
            return dispatch_request (mob);
        case RESPONSE:
            //g_debug ("Received response");
            return dispatch_response (mob);
        case NOTIFY:
            //g_debug ("Received notify");
            return dispatch_notify (mob);
        default:
            return object_error (g_strdup_printf (
                    _("unknown msgpack message type %d"), type));
    }
    return false;
}

bool MsgpackRpc::dispatch_request (const msgpack::object &msg)
{
    const msgpack::object_array ar = msg.via.array;
    if (ar.size != 4)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request should have 4 elements, got %d"),
                        ar.size));
    }
    if (ar.ptr[1].type != msgpack::type::POSITIVE_INTEGER)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request msgid field should be "
                        "POSITIVE_INTEGER, received type %d"),
                        ar.ptr[1].type));
        return false;
    }
    if (ar.ptr[2].type != msgpack::type::STR)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request method field should be "
                        "STR, received type %d"),
                        ar.ptr[2].type));
        return false;
    }
    if (ar.ptr[3].type != msgpack::type::ARRAY)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request args field should be "
                        "ARRAY, received type %d"),
                        ar.ptr[3].type));
        return false;
    }

    if (!this->stop_)
    {
        guint32 msgid = ar.ptr[1].via.i64;
        std::string method;
        ar.ptr[2].convert (method);
        request_signal_.emit (msgid, method, ar.ptr[3]);
    }

    return true;
}

bool MsgpackRpc::dispatch_response (const msgpack::object &msg)
{
    const auto &ar = msg.via.array;
    guint32 msgid = ar.ptr[1].via.i64;
    auto it = response_promises_.find (msgid);
    if (it == response_promises_.end ())
    {
        g_critical ("Response with unexpected msgid %d", msgid);
        return false;
    }
    std::shared_ptr<MsgpackPromise> promise = it->second;

    const auto &error = ar.ptr[2];
    const auto &response = ar.ptr[3];
    if (!this->stop_)
    {
        if (error.is_nil ())
            promise->emit_error (error);
        else
            promise->emit_value (response);
        this->response_promises_.erase (msgid);
    }

    return true;
}

bool MsgpackRpc::dispatch_notify (const msgpack::object &msg)
{
    const auto &ar = msg.via.array;
    if (ar.size != 3)
    {
        return object_error (g_strdup_printf (
                    _("msgpack notify should have 3 elements, got %d"),
                        ar.size));
    }
    if (ar.ptr[1].type != msgpack::type::STR)
    {
        return object_error (g_strdup_printf (
                    _("msgpack notify method field should be "
                        "STR, received type %d"),
                        ar.ptr[1].type));
        return false;
    }
    if (ar.ptr[2].type != msgpack::type::ARRAY)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request args field should be "
                        "ARRAY, received type %d"),
                        ar.ptr[2].type));
        return false;
    }

    if (!this->stop_)
    {
        std::string method;
        ar.ptr[1].convert (method);
        notify_signal_.emit (method, ar.ptr[2]);
    }

    return true;
}

guint32 MsgpackRpc::msgid_ = 0;

}
