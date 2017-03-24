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

#include <utility>

#include "buffer.h"
#include "nvim-bridge.h"

namespace Gnvim
{

Buffer::Buffer (NvimBridge &nvim, int columns, int rows)
        : nvim_ (nvim), columns_ (columns), rows_ (rows),
        fg_colour_ (-1), bg_colour_ (-1), sp_colour_ (-1),
        cursor_row_ (0), cursor_col_ (0)
{
    current_attr_tag_ = default_attr_tag_ = Gtk::TextTag::create ("default");
    get_tag_table ()->add (default_attr_tag_);
    on_nvim_clear ();

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
    nvim.nvim_clear.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_clear));
    nvim.nvim_eol_clear.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_eol_clear));
    nvim.nvim_highlight_set.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_highlight_set));
    nvim.nvim_redraw_end.connect
            (sigc::mem_fun (this, &Buffer::on_nvim_redraw_end));
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
            this->insert_with_tag (end_it, s, current_attr_tag_);
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
            this->insert_with_tag (it, s, current_attr_tag_);
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
    {
        prop.reset_value ();
    }
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
            this->insert_with_tag (end (), s, current_attr_tag_);
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
    this->insert_with_tag (get_cursor_iter (), s, current_attr_tag_);
}

void Buffer::on_nvim_update_fg (int colour)
{
    set_colour_prop (default_attr_tag_->property_foreground_rgba (),
                fg_colour_ = colour);
}

void Buffer::on_nvim_update_bg (int colour)
{
    set_colour_prop (default_attr_tag_->property_background_rgba (),
                bg_colour_ = colour);
}

void Buffer::on_nvim_update_sp (int colour)
{
    set_colour_prop (default_attr_tag_->property_underline_rgba (),
                sp_colour_ = colour);
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
    this->insert_with_tag (get_cursor_iter (), s, current_attr_tag_);
    cursor = get_cursor_iter ();
    cursor.forward_chars (len);
    cursor_col_ = cursor.get_line_offset ();
    cursor_row_ = cursor.get_line ();
}

static Glib::ustring repr_colour (char prefix, int rgb)
{
    auto r = g_strdup_printf ("c%c%02x%02x%02x", prefix,
            (rgb & 0xff0000) >> 16, (rgb & 0xff00) >> 8, rgb & 0xff);
    auto s = Glib::ustring (r);
    g_free (r);
    return s;
}

/*
static int int_from_rgba_prop (const Glib::PropertyProxy<Gdk::RGBA> &prop)
{
    auto rgba = prop.get_value ();
    return (int(rgba.get_red_u () & 0xff00) << 8)
        | int(rgba.get_green_u () & 0xff00)
        | (int(rgba.get_blue_u () & 0xff00) >> 8);
}
*/

void Buffer::on_nvim_highlight_set (const msgpack::object &map_o)
{
    // We could probably share tag tables between multiple windows, but
    // different vim instances may have different themes with different default
    // colours etc, which would make sharing harder
    if (map_o.type != msgpack::type::MAP)
    {
        g_critical ("Got sent type %d as arg for highlight_set, expected MAP",
                map_o.type);
        return;
    }
    const auto &map_m = map_o.via.map;
    int foreground = -1, background = -1, special = -1;
    bool reverse = false;
    bool italic = false;
    bool bold = false;
    bool underline = false;
    bool undercurl = false;
    for (guint n = 0; n < map_m.size; ++n)
    {
        const auto &kv = map_m.ptr[n];
        std::string k;
        kv.key.convert (k);
        if (k == "foreground")
            kv.val.convert (foreground);
        else if (k == "background")
            kv.val.convert (background);
        else if (k == "special")
            kv.val.convert (special);
        else if (k == "reverse")
            kv.val.convert (reverse);
        else if (k == "italic")
            kv.val.convert (italic);
        else if (k == "bold")
            kv.val.convert (bold);
        else if (k == "underline")
            kv.val.convert (underline);
        else if (k == "undercurl")
            kv.val.convert (undercurl);
    }

    if (foreground == -1)
        foreground = fg_colour_;
    if (background == -1)
        background = bg_colour_;
    if (special == -1)
        special = sp_colour_;

    Glib::ustring tag_name;
    if (foreground != -1)
        tag_name += repr_colour ('f', foreground);
    if (background != -1)
        tag_name += repr_colour ('b', background);
    if (special != -1)
        tag_name += repr_colour ('b', special);
    if (reverse)
        tag_name += 'r';
    if (italic)
        tag_name += 'i';
    if (bold)
        tag_name += 'b';
    if (underline)
        tag_name += 'l';
    if (undercurl)
        tag_name += 'w';

    auto table = get_tag_table ();
    auto tag = table->lookup (tag_name);
    if (!tag)
    {
        tag = Gtk::TextTag::create (tag_name);
        if (reverse)
        {
            if (background == -1)
            {
                tag->property_foreground_rgba ().set_value (
                        default_attr_tag_->property_background_rgba
                            ().get_value ());
            }
            else
            {
                set_colour_prop (tag->property_foreground_rgba (), background);
            }
            if (foreground == -1)
            {
                tag->property_background_rgba ().set_value (
                        default_attr_tag_->property_foreground_rgba
                            ().get_value ());
            }
            else
            {
                set_colour_prop (tag->property_background_rgba (), foreground);
            }
        }
        else
        {
            set_colour_prop (tag->property_foreground_rgba (), foreground);
            set_colour_prop (tag->property_background_rgba (), background);
        }
        set_colour_prop (tag->property_underline_rgba (), special);
        tag->property_weight ().set_value (
                bold ? Pango::WEIGHT_BOLD : Pango::WEIGHT_NORMAL);
        tag->property_style ().set_value (
                italic ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL);
        tag->property_underline ().set_value (
                undercurl ? Pango::UNDERLINE_ERROR :
                (underline ? Pango::UNDERLINE_SINGLE : Pango::UNDERLINE_NONE));
        table->add (tag);
    }
    current_attr_tag_ = tag;
}

void Buffer::on_nvim_redraw_end ()
{
    place_cursor (get_cursor_iter ());
}

}
