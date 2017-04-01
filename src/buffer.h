/* buffer.h
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

#ifndef GNVIM_BUFFER_H
#define GNVIM_BUFFER_H

#include "defns.h"

#include <msgpack.hpp>

namespace Gnvim
{

// The buffer is kept filled, using spaces as padding, with each line being
// columns long plus a terminator (\n). The last line doesn't have a terminator.
class Buffer : public Gtk::TextBuffer {
public:
    static Buffer *create (NvimBridge &nvim, int columns, int rows)
    {
        return new Buffer (nvim, columns, rows);
    }

    int get_columns () const
    {
        return columns_;
    }

    int get_rows () const
    {
        return rows_;
    }

    void get_size (int &columns, int &rows) const
    {
        columns = columns_;
        rows = rows_;
    }

    NvimBridge &get_nvim_bridge ()
    {
        return nvim_;
    }

    // Returns false if the size hasn't changed
    bool resize (int columns, int rows);

    void on_redraw_start ();
    void on_redraw_update_fg (int colour);
    void on_redraw_update_bg (int colour);
    void on_redraw_update_sp (int colour);
    void on_redraw_cursor_goto (int row, int col);
    void on_redraw_put (const msgpack::object_array &);
    void on_redraw_clear ();
    void on_redraw_eol_clear ();
    void on_redraw_highlight_set (const msgpack::object &map_o);
    void on_redraw_set_scroll_region (int top, int bot, int left, int right);
    void on_redraw_scroll (int count);
    void on_redraw_end ();

protected:
    Buffer (NvimBridge &nvim, int columns, int rows);
private:
    Gtk::TextIter get_cursor_iter ()
    {
        return get_iter_at_line_offset (cursor_row_, cursor_col_);
    }

    NvimBridge &nvim_;

    int columns_, rows_;

    int fg_colour_, bg_colour_, sp_colour_;
    RefPtr<Gtk::TextTag> default_attr_tag_;
    RefPtr<Gtk::TextTag> current_attr_tag_;
    int cursor_row_, cursor_col_;
    struct {
        int top, bot, left, right;
    } scroll_region_;
};

}

#endif // GNVIM_BUFFER_H
