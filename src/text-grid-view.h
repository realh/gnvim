/* text-grid-view.h
 *
 * Copyright(C) 2017 Tony Houghton
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

class TextGridWidget;

/// Renders the TextGrid to a Cairo surface
class TextGridView {
public:
    TextGridView(int columns, int lines, const std::string &font_name = "");

    virtual ~TextGridView();

    void set_current_widget(Gtk::Widget *w);

    int get_allocated_columns() const
    {
        return columns_;
    }

    int get_allocated_lines() const
    {
        return lines_;
    }

    void get_allocated_grid_size(int &columns, int &lines) const
    {
        columns = columns_;
        lines = lines_;
    }

    /// @param pc       For calculating font size.
    /// @param desc     A pango font description string or "" for default.
    /// @param resize   Whether to resize the surface.
    void set_font(RefPtr<Pango::Context> pc,
            const Glib::ustring &desc = "", bool resize = true);

    TextGrid &get_grid()
    {
        return grid_;
    }

    // in = window coords eg from mouse event, out = vim text coords
    void convert_coords_to_cells(int &x, int &y)
    {
        x /= cell_width_px_;
        y /= cell_height_px_;
    }

    // Clears an area of text and the cached cairo surface. Limits are inclusive
    // in units of text cells.
    void clear(int left, int top, int right, int bottom)
    {
        grid_.clear(left, top, right, bottom);
        if (!global_redraw_pending_)
            fill_background(grid_cr_, left, top, right, bottom);
    }

    // Clear the entire view.
    void clear()
    {
        grid_.clear();
        if (!global_redraw_pending_)
            clear_view();
    }

    // Parameters are same as for TextGrid::Scroll, which this method calls
    // in addition to updating the view.
    void scroll(int left, int top, int right, int bottom, int count);

    // @param w A widget this will be rendered in, provides render context info.
    void create_cairo_surface(Gtk::Widget *w);

    // Called from widget's namesake.
    virtual void on_size_allocate(Gtk::Allocation &alloc);

    // Called by widget's vfunc.
    void get_preferred_width(int &minimum, int &natural) const;

    // Called by widget's vfunc.
    void get_preferred_height(int &minimum, int &natural) const;

    void show_cursor();

    const Cairo::RefPtr<Cairo::Surface> get_surface() const
    {
        return grid_surface_;
    }

    // Fills the given region(inclusive text cells) with the given rgb colour.
    void fill_background(Cairo::RefPtr<Cairo::Context> cr, int left, int top,
            int right, int bottom, guint32 rgb);

    // Fills the given region(inclusive text cells) with the background
    // colour.
    void fill_background(Cairo::RefPtr<Cairo::Context> cr, int left, int top,
            int right, int bottom)
    {
        fill_background(cr, left, top, right, bottom,
                grid_.get_default_background_rgb());
    }

    // Fills a Cairo context with the background colour,
    // assuming size == allocation
    void fill_background(Cairo::RefPtr<Cairo::Context> cr)
    {
        fill_background(cr, 0, 0, columns_ - 1, lines_ - 1);
    }

    // As fill_background but in pixel units
    void fill_background_px(Cairo::RefPtr<Cairo::Context> cr,
            int left, int top, int width, int height, guint32 rgb);

    void fill_background_px(Cairo::RefPtr<Cairo::Context> cr,
            int left, int top, int width, int height)
    {
        fill_background_px(cr, left, top, width, height,
                grid_.get_default_background_rgb());
    }

    // Draws or erases cursor depending on its visibility. cr is usually the
    // widget's surface, the cursor shouldn't be drawn onto the cached surface.
    void draw_cursor(Cairo::RefPtr<Cairo::Context> cr);
protected:
    void resize_surface();

    /// Blanks the entire cached view in the grid's background colour.
    void clear_view()
    {
        fill_background(grid_cr_);
    }

    /// Redraws the entire cached view (starting with clear_view() ).
    void redraw_view();

    void on_toplevel_size_allocate(Gtk::Allocation &);

    void calculate_metrics(RefPtr<Pango::Context> pc);

    TextGrid grid_;

    TextGridWidget *current_widget_ {nullptr};

    int cell_width_px_, cell_height_px_;
    int columns_, lines_;

    int toplevel_width_ {0}, toplevel_height_ {0};

    Pango::FontDescription font_;
    constexpr static const char *DEFAULT_FONT = "Monospace 11";

    // For caching the grid display
    Cairo::RefPtr<Cairo::Surface> grid_surface_;
    Cairo::RefPtr<Cairo::Context> grid_cr_;

    // If true we needn't bother updating cached view at every change
    bool global_redraw_pending_ {false};

    bool on_cursor_blink();

    void on_cursor_gsetting_changed(const Glib::ustring &key);

    int cursor_col_ {0}, cursor_line_ {0};

    sigc::connection cursor_cx_;
    sigc::connection cursor_blink_cx_, cursor_blink_time_cx_,
        cursor_blink_timeout_cx_;
    bool cursor_visible_ {false};
    bool cursor_blinks_;
    unsigned cursor_interval_;
    guint64 cursor_timeout_;
    gint64 cursor_idle_at_;
    int cursor_width_ {0};      // 0 for block cursor
    CellAttributes cursor_attr_;
    bool colour_cursor_;        // Whether cursor has its own colour or it
                                // depends on fg/bg
};

}

#endif // GNVIM_TEXT_GRID_VIEW_H
