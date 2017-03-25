/* view.h
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

#ifndef GNVIM_VIEW_H
#define GNVIM_VIEW_H

#include "defns.h"

#include "buffer.h"

namespace Gnvim
{

class View : public Gtk::TextView {
public:
    // Want to be passed an unref'd pointer because super takes a ref, but
    // it's a ref to Gtk::TextBuffer
    View (Buffer *buffer);

    // Height includes spacing
    void get_cell_size_in_pixels (int &width, int &height) const
    {
        width = cell_width_px_;
        height = cell_height_px_;
    }

    void get_allocation_in_cells (int &columns, int &rows) const
    {
        columns = columns_;
        rows = rows_;
    }

    // width and height are out parameters
    void get_preferred_size (int &width, int &height);

    // width and height are columns and rows in, pixels out
    void get_preferred_size_for (int &width, int &height);

    // Sets (supposed) size passively. Actual widget resizing should be done
    // separately eg by resizing parent window
    void set_grid_size (int columns, int rows)
    {
        columns_ = columns;
        rows_ = rows;
    }
protected:
    virtual void on_size_allocate (Gtk::Allocation &) override;

    virtual bool on_key_press_event (GdkEventKey *) override;
private:
    void calculate_metrics ();

    Buffer *buffer_;

    int cell_width_px_, cell_height_px_;
    int columns_, rows_;
};

}

#endif // GNVIM_VIEW_H
