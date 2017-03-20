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
    rcv_thread_ = std::thread([this] () { this->run_rcv_thread (); });
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
    stop_.store (true);
    rcv_cancellable_->cancel ();
    if (rcv_thread_.joinable ())
        rcv_thread_.join ();
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

void MsgpackRpc::run_rcv_thread ()
{
    msgpack::unpacker unpacker;

    while (!stop_.load ())
    {
        std::string buf (BUFLEN, 0);
        gsize nread;
        Glib::ustring error_msg;

        unpacker.reserve_buffer (BUFLEN);
        try
        {
            do
            {
                nread = strm_from_nvim_->read (unpacker.buffer (), BUFLEN, 
                        rcv_cancellable_);
            } while (!nread && !rcv_cancellable_->is_cancelled () &&
                    !stop_.load ());
        }
        catch (Glib::Exception &e)
        {
            error_msg = e.what ();
            nread = 0;
        }
        if (!nread)
        {
            if (!error_msg.length ())
                error_msg = _("No bytes read");
            if (!stop_.load())
            {
                stop_.store (true);
                reference ();
                Glib::signal_idle ().connect_once ([this, error_msg] ()
                {
                    rcv_error_signal_.emit (error_msg);
                    unreference ();
                });
            }
            break;
        }
        unpacker.buffer_consumed (nread);

        msgpack::unpacked unpacked;
        while (!stop_.load () && unpacker.next (unpacked))
        {
            object_received (unpacked.get ());
        }
    }
    g_debug ("rcv thread stopped");
}

bool MsgpackRpc::object_error (char *raw_msg)
{
    stop_.store (true);
    auto msg = Glib::ustring (raw_msg);
    g_free (raw_msg);
    reference ();
    Glib::signal_idle ().connect_once ([this, msg] () {
        rcv_error_signal ().emit (msg);
    });
    return false;
}

static std::string msgpack_to_str (const msgpack::object &o)
{
    return std::string(o.via.str.ptr, o.via.str.ptr + o.via.str.size);
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
            return dispatch_request (array);
        case RESPONSE:
            return dispatch_response (array.ptr[1].via.i64,
                    array.ptr[2], array.ptr[3]);
        case NOTIFY:
            return dispatch_notify (array);
        default:
            return object_error (g_strdup_printf (
                    _("unknown msgpack message type %d"), type));
    }
    return false;
}

bool MsgpackRpc::dispatch_request (const msgpack::object_array &msg)
{
    if (msg.size != 4)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request should have 4 elements, got %d"),
                        msg.size));
    }
    if (msg.ptr[1].type != msgpack::type::POSITIVE_INTEGER)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request msgid field should be "
                        "POSITIVE_INTEGER, received type %d"),
                        msg.ptr[1].type));
        return false;
    }
    if (msg.ptr[2].type != msgpack::type::STR)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request method field should be "
                        "STR, received type %d"),
                        msg.ptr[2].type));
        return false;
    }
    if (msg.ptr[3].type != msgpack::type::ARRAY)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request args field should be "
                        "ARRAY, received type %d"),
                        msg.ptr[3].type));
        return false;
    }
    reference ();
    Glib::signal_idle ().connect_once ([this, msg] () {
        if (!this->stop_.load ())
        {
            request_signal_.emit (
                    msg.ptr[1].via.i64,
                    msgpack_to_str (msg.ptr[2]),
                    msg.ptr[3]);
        }
        unreference ();
    });
    return true;
}

bool MsgpackRpc::dispatch_response (guint32 msgid,
            msgpack::object error, msgpack::object response)
{
    auto it = response_promises_.find (msgid);
    if (it == response_promises_.end ())
    {
        g_critical ("Response with unexpected msgid %d", msgid);
        return false;
    }
    std::shared_ptr<MsgpackPromise> promise = it->second;

    reference ();
    Glib::signal_idle ().connect_once (
            [this, msgid, promise, error, response] ()
    {
        if (!this->stop_.load ())
        {
            if (!error.is_nil ())
                promise->emit_error (error);
            else
                promise->emit_value (response);
            this->response_promises_.erase (msgid);
        }
        unreference ();
    });

    return true;
}

bool MsgpackRpc::dispatch_notify (const msgpack::object_array &msg)
{
    if (msg.size != 3)
    {
        return object_error (g_strdup_printf (
                    _("msgpack notify should have 3 elements, got %d"),
                        msg.size));
    }
    if (msg.ptr[1].type != msgpack::type::STR)
    {
        return object_error (g_strdup_printf (
                    _("msgpack notify method field should be "
                        "STR, received type %d"),
                        msg.ptr[1].type));
        return false;
    }
    if (msg.ptr[2].type != msgpack::type::ARRAY)
    {
        return object_error (g_strdup_printf (
                    _("msgpack request args field should be "
                        "ARRAY, received type %d"),
                        msg.ptr[2].type));
        return false;
    }
    reference ();
    Glib::signal_idle ().connect_once ([this, msg] () {
        if (!this->stop_.load ())
        {
            notify_signal_.emit (
                    msgpack_to_str (msg.ptr[1]),
                    msg.ptr[2]);
        }
        unreference ();
    });
    return true;
}

guint32 MsgpackRpc::msgid_ = 0;

}
