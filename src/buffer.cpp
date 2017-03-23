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
        : nvim_ (nvim), columns_ (columns), rows_ (rows)
{
    current_attr_tag_ = default_attr_tag_ = Gtk::TextTag::create ();
    init_content ();
    cursor_ = begin ();

    nvim.nvim_update_bg.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_update_bg));
    nvim.nvim_update_fg.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_update_fg));
    nvim.nvim_update_sp.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_update_sp));
    nvim.nvim_cursor_goto.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_cursor_goto));
}

void Buffer::init_content ()
{
    Glib::ustring s;

    for (int y = 0; y < rows_; ++y)
    {
        s = Glib::ustring (columns_, ' ');
        if (y < rows_ - 1)
            s += '\n';
        if (y == 0)
            set_text (s);
        else
            insert (end (), s);
    }
    apply_tag (default_attr_tag_, begin (), end ());
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
    cursor_.set_line (row);
    cursor_.set_line_offset (col);
}

}
