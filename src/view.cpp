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

bool View::on_key_press_event (GdkEventKey *event)
{
    if (event->is_modifier)
    {
        return true;
    }

    bool special = false;
    gunichar uk = gdk_keyval_to_unicode (event->keyval);
    Glib::ustring s = gdk_keyval_name (event->keyval);

    bool kp = s.find ("KP_") == 0;
    if (kp)
    {
        special = true;
        s = s.substr (3);
        if (s == "Add")
            s = "Plus";
        else if (s == "Subtract")
            s = "Minus";
        else if (s == "Decimal")
            s = "Point";
        s = Glib::ustring (1, 'k') + s;
    }

    if (g_unichar_isprint (uk))
    {
        s = Glib::ustring (1, uk);
        if (s == "<")
        {
            special = true;
            s = "lt";
        }
        else if (s == "\\")
        {
            special = true;
            s = "Bslash";
        }
    }
    else
    {
        special = true;
        if (s == "BackSpace")
            s = "BS";
        else if (s == "Return" || s == "Enter")
            s = "CR";
        else if (s == "Escape")
            s = "Esc";
        else if (s == "Delete")
            s = "Del";
        else if (s == "Page_Up")
            s = "PageUp";
        else if (s == "Page_Down")
            s = "PageDown";
        else if (s == "ISO_Left_Tab")
            s = "Tab";
    }

    if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
        special = true;
    if (event->state & GDK_SHIFT_MASK)
        s = Glib::ustring ("S-") + s;
    if (event->state & GDK_CONTROL_MASK)
        s = Glib::ustring ("C-") + s;
    if (event->state & GDK_MOD1_MASK)
        s = Glib::ustring ("A-") + s;

    if (special)
        s = Glib::ustring (1, '<') + s + '>';

    //g_debug("Keypress %s", s.c_str ());

    buffer_->get_nvim_bridge ().nvim_feedkeys (s);
    return true;
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
