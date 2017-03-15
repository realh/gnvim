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

namespace Gnvim
{

class View : public Gtk::TextView {
public:
    View (int columns, int rows);

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

    void get_preferred_size (int &width, int &height);
protected:
    virtual void on_size_allocate (Gtk::Allocation &) override;
private:
    void calculate_metrics ();

    int cell_width_px_, cell_height_px_;
    int columns_, rows_;
};

}

#endif // GNVIM_VIEW_H
