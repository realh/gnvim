/* window.cpp
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

#include "view.h"
#include "window.h"

namespace Gnvim
{

Window::Window (std::vector<const char *>)
{
    view_ = new View (80, 36);
    int width, height;
    view_->get_preferred_size (width, height);
    set_default_size (width, height);

    nvim_.error_signal ().connect (
            sigc::mem_fun (*this, &Window::on_nvim_error));
    nvim_.start ();

    view_->show_all ();
    add (*view_);
    set_geometry_hints ();
    if (!force_close_)
        present ();
    else
        g_warning ("Window closed before it opened");
}

void Window::force_close ()
{
    force_close_ = true;
    close ();
}

void Window::on_nvim_error (Glib::ustring desc)
{
    g_critical ("Closing window due to nvim communication error: %s",
            desc.c_str ());
    force_close ();
}

#if 0
void Window::set_geometry_hints ()
{
    Gdk::Geometry geom;
    view_->get_cell_size_in_pixels (geom.width_inc, geom.height_inc);
    // Use max_ as placeholder for total size
    view_->get_preferred_size (geom.max_width, geom.max_height);
    geom.max_width += get_margin_left () + get_margin_right ();
    geom.max_height += get_margin_top () + get_margin_bottom ();
    // Use min_ as placeholder for number of cells
    view_->get_allocation_in_cells (geom.min_width, geom.min_height);
    geom.base_width = geom.max_width - geom.min_width * geom.width_inc;
    geom.base_height = geom.max_height - geom.min_height * geom.height_inc;
    geom.min_width = 5 * geom.width_inc + geom.base_width;
    geom.min_height = 4 * geom.height_inc + geom.base_height;
    g_debug ("Base size %dx%d, increments %dx%d",
            geom.base_width, geom.base_height,
            geom.width_inc, geom.height_inc);
    Gtk::ApplicationWindow::set_geometry_hints (*view_, geom,
            Gdk::HINT_MIN_SIZE | Gdk::HINT_RESIZE_INC | Gdk::HINT_BASE_SIZE);
}
#endif

}
