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

MsgpackRpc::MsgpackRpc (int pipe_to_nvim, int pipe_from_nvim)
        : strm_to_nvim_ (Gio::UnixOutputStream::create (pipe_to_nvim, TRUE)),
        strm_from_nvim_ (Gio::UnixInputStream::create (pipe_from_nvim, TRUE)),
        send_thread_ ([this] () { this->run_send_thread (); }),
        rcv_thread_ ([this] () { this->run_rcv_thread (); })
{
}

MsgpackRpc::~MsgpackRpc ()
{
    stop ();
}

void MsgpackRpc::stop ()
{
    stop_.store (true);
    send_cancellable_->cancel ();
    send_cond_.notify_all ();
    rcv_cancellable_->cancel ();
    if (send_thread_.joinable ())
        send_thread_.join ();
    if (rcv_thread_.joinable ())
        rcv_thread_.join ();
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
    packer.pack_array (3);
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
    std::lock_guard<std::mutex> lock (send_mutex_);
    send_queue_.push_back (s);
    send_cond_.notify_one ();
}

void MsgpackRpc::run_send_thread ()
{
    std::unique_lock<std::mutex> lock (send_mutex_);

    while (!stop_.load ())
    {
        if (!send_queue_.size ())
        {
            send_cond_.wait (lock);
        }
        // May be woken even if no message has been pushed
        if (send_queue_.size () && !stop_.load ())
        {
            std::string msg = send_queue_.front ();
            send_queue_.pop_front ();
            lock.unlock ();
            gsize nwritten = 0;
            Glib::ustring error_msg;
            try
            {
                if (strm_to_nvim_->write_all (msg,
                            nwritten, send_cancellable_))
                {
                    strm_to_nvim_->flush (send_cancellable_);
                }
                else
                {
                    nwritten = 0;
                }
            }
            catch (Glib::Exception &e)
            {
                error_msg = e.what ();
                nwritten = 0;
            }
            if (!nwritten)
            {
                if (!stop_.load ())
                {
                    stop_.store (true);
                    reference ();
                    Glib::signal_idle ().connect_once ([this, error_msg] ()
                    {
                        send_error_signal_.emit (error_msg);
                        unreference ();
                    });
                }
                break;
            }
            else
            {
                lock.lock ();
            }
        }
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

        try
        {
            do
            {
                nread = strm_from_nvim_->read (
                        const_cast<char *> (buf.data ()), BUFLEN, 
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
        while (!stop_.load() && unpacker.next (unpacked))
        {
            object_received (unpacked.get ());
        }
    }
}

bool MsgpackRpc::object_error (char *raw_msg)
{
    stop_.store (true);
    auto msg = Glib::ustring (raw_msg);
    g_free (raw_msg);
    rcv_error_signal ().emit (msg);
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
            return dispatch_response (array);
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
        request_signal_.emit (
                msg.ptr[1].via.i64,
                msgpack_to_str (msg.ptr[2]),
                msg.ptr[3].via.array);
        unreference ();
    });
    return true;
}

bool MsgpackRpc::dispatch_response (const msgpack::object_array &msg)
{
    if (msg.size != 4)
    {
        return object_error (g_strdup_printf (
                    _("msgpack response should have 4 elements, got %d"),
                        msg.size));
    }
    if (msg.ptr[1].type != msgpack::type::POSITIVE_INTEGER)
    {
        return object_error (g_strdup_printf (
                    _("msgpack response msgid field should be "
                        "POSITIVE_INTEGER, received type %d"),
                        msg.ptr[1].type));
    }

    std::lock_guard<std::mutex> lock (response_mutex_);
    response_msgid_ = msg.ptr[1].via.i64;
    if (pending_responses_.find (response_msgid_) == pending_responses_.end ())
    {
        g_warning (_("Response to unknown msgid %d"), response_msgid_);
        return true;
    }

    if (!msg.ptr[2].is_nil ())
        response_ = new msgpack::object (msg.ptr[2]);
    if (!msg.ptr[3].is_nil ())
        response_error_ = new msgpack::object (msg.ptr[3]);

    response_cond_.notify_all ();

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
        notify_signal_.emit (
                msgpack_to_str (msg.ptr[1]),
                msg.ptr[2].via.array);
        unreference ();
    });
    return true;
}

guint32 MsgpackRpc::msgid_ = 0;

}
