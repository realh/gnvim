/* text-grid-view.h
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

#ifndef GNVIM_TEXT_GRID_VIEW_H
#define GNVIM_TEXT_GRID_VIEW_H

#include "defns.h"

#include "text-grid.h"

namespace Gnvim
{

class TextGridView : public Gtk::DrawingArea {
public:
    TextGridView (int columns, int lines, const std::string &font_name = "");

    int get_allocated_columns () const
    {
        return columns_;
    }

    int get_allocated_lines () const
    {
        return lines_;
    }

    void get_allocated_grid_size (int &columns, int &lines) const
    {
        columns = columns_;
        lines = lines_;
    }

    // desc is a pango font description string or "" for default
    void set_font (const Glib::ustring &desc = "", bool q_resize = true);

    TextGrid &get_grid ()
    {
        return grid_;
    }
protected:
    virtual void on_parent_changed (Gtk::Widget *old_parent) override;

    virtual void on_size_allocate (Gtk::Allocation &) override;

    virtual void get_preferred_width_vfunc (int &minimum, int &natural)
            const override;

    virtual void get_preferred_height_vfunc (int &minimum, int &natural)
            const override;

    TextGrid grid_;
private:
    void on_toplevel_size_allocate (Gtk::Allocation &);

    void calculate_metrics ();

    // Try to resize the parent window to fit the desired number of columns
    // and lines
    void resize_window ();

    // in = window coords eg from mouse event, out = vim text coords
    void convert_coords_to_cells (int &x, int &y)
    {
        x /= cell_width_px_;
        y /= cell_height_px_;
    }

    int cell_width_px_, cell_height_px_;
    int columns_, lines_;

    sigc::connection toplevel_size_allocate_connection_;
    int toplevel_width_ {0}, toplevel_height_ {0};

    Pango::FontDescription font_;
    constexpr static const char DEFAULT_FONT[] = "Monospace 11";
};

}

#endif // GNVIM_TEXT_GRID_VIEW_H
