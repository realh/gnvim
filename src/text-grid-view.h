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

#include <utility>
#include <vector>

#include "text-grid.h"

namespace Gnvim
{

/// Displays the TextGrid
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

    // Try to resize the parent window to fit the desired number of columns
    // and lines
    void resize_window ();

    // in = window coords eg from mouse event, out = vim text coords
    void convert_coords_to_cells (int &x, int &y)
    {
        x /= cell_width_px_;
        y /= cell_height_px_;
    }

    // Clears an area of text and the cached cairo surface. Limits are inclusive
    // in units of text cells.
    void clear (int left, int top, int right, int bottom)
    {
        grid_.clear (left, top, right, bottom);
        fill_background (grid_cr_, left, top, right, bottom);
    }

    // Clear the entire view.
    void clear ()
    {
        grid_.clear ();
        clear_view ();
    }

    // Parameters are same as for TextGrid::Scroll, which this method calls
    // in addition to updating the view.
    void scroll (int left, int top, int right, int bottom, int count);
protected:
    virtual void on_realize () override;

    virtual bool on_draw (const Cairo::RefPtr<Cairo::Context> &cr) override;

    virtual void on_parent_changed (Gtk::Widget *old_parent) override;

    virtual void on_size_allocate (Gtk::Allocation &) override;

    virtual void get_preferred_width_vfunc (int &minimum, int &natural)
            const override;

    virtual void get_preferred_height_vfunc (int &minimum, int &natural)
            const override;

    void create_cairo_surface ();

    // Fills the given region (inclusive text cells) with the background
    // colour.
    void fill_background (Cairo::RefPtr<Cairo::Context> cr, int left, int top,
            int right, int bottom);

    // Fills a Cairo context with the background colour,
    // assuming size == allocation
    void fill_background (Cairo::RefPtr<Cairo::Context> cr)
    {
        fill_background (cr, 0, 0, columns_ - 1, lines_ - 1);
    }

    /// Blanks the entire cached view in the grid's background colour.
    void clear_view ()
    {
        fill_background (grid_cr_);
    }

    /// Redraws the entire cached view (starting with clear_view ()).
    void redraw_view ();

    void on_toplevel_size_allocate (Gtk::Allocation &);

    void calculate_metrics ();

    TextGrid grid_;

    int cell_width_px_, cell_height_px_;
    int columns_, lines_;

    sigc::connection toplevel_size_allocate_connection_;
    int toplevel_width_ {0}, toplevel_height_ {0};

    Pango::FontDescription font_;
    constexpr static const char *DEFAULT_FONT = "Monospace 11";

    // vector has an element per grid line, first = left, second = right
    // (inclusive); -1 for no redraw queued
    std::vector<std::pair<int, int>> changed_lines_;

    // For caching the grid display
    Cairo::RefPtr<Cairo::Surface> grid_surface_;
    Cairo::RefPtr<Cairo::Context> grid_cr_;
};

}

#endif // GNVIM_TEXT_GRID_VIEW_H
