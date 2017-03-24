/* buffer.cpp
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

#include "buffer.h"
#include "nvim-bridge.h"

namespace Gnvim
{

Buffer::Buffer (NvimBridge &nvim, int columns, int rows)
        : nvim_ (nvim), columns_ (columns), rows_ (rows),
        cursor_row_ (0), cursor_col_ (0)
{
    on_nvim_clear ();
    current_attr_tag_ = default_attr_tag_ = Gtk::TextTag::create ();

    nvim.nvim_update_bg.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_update_bg));
    nvim.nvim_update_fg.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_update_fg));
    nvim.nvim_update_sp.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_update_sp));
    nvim.nvim_cursor_goto.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_cursor_goto));
    nvim.nvim_put.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_put));
    nvim.nvim_redraw_end.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_redraw_end));
    nvim.nvim_clear.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_clear));
    nvim.nvim_eol_clear.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_eol_clear));
}

bool Buffer::resize (int columns, int rows)
{
    if (columns == columns_ && rows == rows_)
        return false;
    if (rows_ > rows)
    {
        // Delete superfluous lines
        Gtk::TextIter start_it = get_iter_at_line (rows);
        Gtk::TextIter end_it = end ();
        // Including terminator of new last line
        start_it.backward_char ();
        erase (start_it, end_it);
        rows_ = rows;
    }
    else if (rows_ < rows)
    {
        Glib::ustring s ("\n");
        s += Glib::ustring (columns_, ' ');
        for (int y = 0; y < rows - rows_; ++y)
        {
            Gtk::TextIter end_it = end ();
            insert (end_it, s);
        }
    }
    // rows_ is now the number of rows that (may) need their length changed
    if (columns > columns_)
    {
        Glib::ustring s (columns - columns_, ' ');
        for (int y = 0; y < rows_; ++y)
        {
            Gtk::TextIter it = get_iter_at_line_offset (y, columns_ - 1);
            it.forward_char ();
            insert (it, s);
        }
    }
    else if (columns < columns_)
    {
        for (int y = 0; y < rows_; ++y)
        {
            Gtk::TextIter start_it = get_iter_at_line_offset (y, columns);
            Gtk::TextIter end_it (start_it);
            end_it.forward_to_line_end ();
            erase (start_it, end_it);
        }
    }

    columns_ = columns;
    rows_ = rows;
    return true;
}

static void set_colour_prop (Glib::PropertyProxy<Gdk::RGBA> prop, int colour)
{
    if (colour == -1)
        prop.reset_value ();
    else
    {
        static Gdk::RGBA rgba;

        rgba.set_rgba_u ((colour & 0xff0000) >> 8,
                colour & 0xff00,
                (colour & 0xff) << 8);
        prop.set_value (rgba);
    }
}

void Buffer::on_nvim_clear ()
{
    for (int y = 0; y < rows_; ++y)
    {
        auto s = Glib::ustring (columns_, ' ');
        if (y < rows_ - 1)
            s += '\n';
        if (y == 0)
            set_text (s);
        else
            insert (end (), s);
    }
}

void Buffer::on_nvim_eol_clear ()
{
    auto cursor = get_cursor_iter ();
    auto range_end = cursor;
    if (range_end.ends_line ())
        return;
    range_end.forward_to_line_end ();
    auto len = range_end.get_line_offset () - cursor.get_line_offset ();
    erase (cursor, range_end);
    auto s = Glib::ustring (len, ' ');
    insert (get_cursor_iter (), s);
}

void Buffer::on_nvim_update_fg (int colour)
{
    set_colour_prop (default_attr_tag_->property_foreground_rgba (), colour);
}

void Buffer::on_nvim_update_bg (int colour)
{
    set_colour_prop (default_attr_tag_->property_background_rgba (), colour);
}

void Buffer::on_nvim_update_sp (int colour)
{
    set_colour_prop (default_attr_tag_->property_underline_rgba (), colour);
}

void Buffer::on_nvim_cursor_goto (int row, int col)
{
    cursor_col_ = col;
    cursor_row_ = row;
//  std::cout << "Moving cursor to row " << row << ", col " << col << std::endl;
}

void Buffer::on_nvim_put (const msgpack::object_array &text_ar)
{
    Glib::ustring s;

    for (gsize i = 1; i < text_ar.size; ++i)
    {
        const auto &o = text_ar.ptr[i];
        if (o.type == msgpack::type::STR)
        {
            const auto &ms = o.via.str;
            s += std::string (ms.ptr, ms.ptr + ms.size);
        }
        else if (o.type == msgpack::type::ARRAY)
        {
            const auto &ma = o.via.array;
            for (gsize j = 0; j < ma.size; ++j)
            {
                const auto &ms = ma.ptr[j].via.str;
                s += std::string (ms.ptr, ms.ptr + ms.size);
            }
        }
    }

    // ustring length () returns number of UTF-8 chars, not length in bytes
    auto len = s.length ();
    auto cursor = get_cursor_iter ();
    auto range_end = cursor;
    range_end.forward_chars (len);
    erase (cursor, range_end);
    cursor = get_cursor_iter ();
    insert (cursor, s);
    cursor = get_cursor_iter ();
    cursor.forward_chars (len);
    cursor_col_ = cursor.get_line_offset ();
    cursor_row_ = cursor.get_line ();
}

void Buffer::on_nvim_redraw_end ()
{
    place_cursor (get_cursor_iter ());
}

}
