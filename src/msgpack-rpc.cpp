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
        stop_ (false),
        send_thread_ ([this] () { this->run_send_thread (); })
{
}

MsgpackRpc::~MsgpackRpc ()
{
    stop_.store (true);
    send_cond_.notify_one ();
    send_thread_.join ();
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
                if (!strm_to_nvim_->write_all (msg, nwritten))
                    nwritten = 0;
            }
            catch (Glib::Exception &e)
            {
                error_msg = e.what ();
                nwritten = 0;
            }
            if (!nwritten)
            {
                stop_.store (true);
                Glib::signal_idle ().connect_once (
                        [this, error_msg] ()
                            { send_error_signal_.emit (error_msg); });
                break;
            }
            else
            {
                lock.lock ();
            }
        }
    }
}

guint32 MsgpackRpc::msgid_ = 0;

}
