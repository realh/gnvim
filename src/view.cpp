/* view.cpp
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

#include "nvim-bridge.h"
#include "view.h"

namespace Gnvim
{

View::View (Buffer *buffer)
        : Gtk::TextView (RefPtr<Gtk::TextBuffer> (buffer)),
        buffer_ (buffer)
{
    // Disabling wrapping should minimise nasty things happening during
    // asynchronous size changes
    set_wrap_mode (Gtk::WRAP_NONE);
    buffer->get_size (columns_, rows_);
    set_monospace ();
    calculate_metrics ();

}

void View::on_size_allocate (Gtk::Allocation &allocation)
{
    Gtk::TextView::on_size_allocate (allocation);
    // Size requests and allocations appear not to include margins
    auto borders_width = get_border_window_size (Gtk::TEXT_WINDOW_LEFT)
            + get_border_window_size (Gtk::TEXT_WINDOW_RIGHT);
    auto borders_height = get_border_window_size (Gtk::TEXT_WINDOW_TOP)
            + get_border_window_size (Gtk::TEXT_WINDOW_BOTTOM);
    columns_ = (allocation.get_width () - borders_width) / cell_width_px_;
    rows_ = (allocation.get_height () - borders_height) / cell_height_px_;

    /*
    g_debug ("allocation %dx%d, grid size %dx%d",
            allocation.get_width (), allocation.get_height (),
            columns_, rows_);
    */

    // Does nothing if size hasn't changed
    buffer_->resize (columns_, rows_);
}

void View::calculate_metrics ()
{
    auto pc = get_pango_context ();
    auto desc = pc->get_font_description ();
    auto metrics = pc->get_metrics (desc);
    // digit_width and char_width should be the same for monospace, but
    // digit_width might be slightly more useful in case we accidentally use
    // a proportional font
    cell_width_px_ = metrics.get_approximate_digit_width () / PANGO_SCALE;
    cell_height_px_ = (metrics.get_ascent () + metrics.get_descent ()
            + get_pixels_above_lines () + get_pixels_below_lines ())
            / PANGO_SCALE;

    // g_debug ("Cell size %dx%d", cell_width_px_, cell_height_px_);
}

void View::get_preferred_size (int &width, int &height)
{
    width = columns_;
    height = rows_;
    get_preferred_size_for (width, height);
}

void View::get_preferred_size_for (int &width, int &height)
{
    width = cell_width_px_ * width
            + get_margin_left () + get_margin_right ();
    height = cell_height_px_ * height
            + get_margin_top () + get_margin_bottom ();
    // g_debug ("Preferred size %dx%d", width, height);
}

}
