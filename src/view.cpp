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

View::View () : buffer_ (nullptr)
{
    // Disabling wrapping should minimise nasty things happening during
    // asynchronous size changes
    set_wrap_mode (Gtk::WRAP_NONE);
    set_monospace ();
    calculate_metrics ();

    on_redraw_mode_change ("normal");

}

View::View (Buffer *buffer) : View ()
{
    set_buffer (buffer);
}

void View::set_buffer (Buffer *buffer)
{
    if (buffer_)
    {
        auto &nvim = buffer_->get_nvim_bridge ();
        nvim.redraw_mode_change.clear ();
        nvim.redraw_bell.clear ();
        nvim.redraw_visual_bell.clear ();
    }
    buffer_ = buffer;
    Gtk::TextView::set_buffer (RefPtr<Gtk::TextBuffer> (buffer));
    auto &nvim = buffer->get_nvim_bridge ();
    nvim.redraw_mode_change.connect (
            sigc::mem_fun (this, &View::on_redraw_mode_change));
    nvim.redraw_bell.connect (sigc::mem_fun (this, &View::on_redraw_bell));
    nvim.redraw_visual_bell.connect
            (sigc::mem_fun (this, &View::on_redraw_bell));
    nvim.redraw_resize.connect
        (sigc::mem_fun (this, &View::on_redraw_resize));
}

Glib::ustring modifier_string (guint state)
{
    std::string s;
    if (state & GDK_MOD1_MASK)
        s = "A-";
    if (state & GDK_CONTROL_MASK)
        s += "C-";
    if (state & GDK_SHIFT_MASK)
        s += "S-";
    return s;
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
    {
        special = true;
        s = modifier_string (event->state) + s;
    }

    if (special)
        s = Glib::ustring (1, '<') + s + '>';

    //g_debug("Keypress %s", s.c_str ());

    buffer_->get_nvim_bridge ().nvim_input (s);
    return true;
}

static const char *mouse_button_name (guint button)
{
    switch (button)
    {
        case 1:
            return "Left";
        case 2:
            return "Middle";
        case 3:
            return "Right";
    }
    return nullptr;
}

bool View::on_mouse_event (GdkEventType etype, int button,
        guint modifiers, int x, int y)
{
    if (!button || button >= 4)
        return true;

    Glib::ustring etype_str;
    switch (etype)
    {
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
            etype_str = "Mouse";
            break;
        case GDK_BUTTON_RELEASE:
            etype_str = "Release";
            break;
        case GDK_MOTION_NOTIFY:
            etype_str = "Drag";
            break;
        default:
            return true;
    }

    Glib::ustring but_str;
    if (button == 1 && etype == GDK_2BUTTON_PRESS)
        but_str = "2-Left";
    else if (button == 1 && etype == GDK_3BUTTON_PRESS)
        but_str = "3-Left";
    else
        but_str = mouse_button_name (button);

    convert_coords_to_cells (x, y);

    char *inp = g_strdup_printf ("<%s%s%s><%d,%d>",
            modifier_string (modifiers).c_str (),
            but_str.c_str(), etype_str.c_str (),
            x, y);
    //g_debug ("Mouse event %s", inp);
    buffer_->get_nvim_bridge ().nvim_input (inp);
    g_free (inp);

    return true;
}

bool View::on_button_press_event (GdkEventButton *event)
{
    return on_mouse_event (event->type, event->button,
            event->state, event->x, event->y);
}

bool View::on_button_release_event (GdkEventButton *event)
{
    return on_mouse_event (event->type, event->button,
            event->state, event->x, event->y);
}

bool View::on_motion_notify_event (GdkEventMotion *event)
{
    auto b = event->state;
    if (b & GDK_BUTTON1_MASK)
        b = 1;
    else if (b & GDK_BUTTON2_MASK)
        b = 2;
    else if (b & GDK_BUTTON3_MASK)
        b = 3;
    else
        return true;
    return on_mouse_event (event->type, b, event->state, event->x, event->y);
}

bool View::on_scroll_event (GdkEventScroll * /*event*/)
{
    return true;
}

int View::get_borders_width () const
{
    return get_border_window_size (Gtk::TEXT_WINDOW_LEFT)
            + get_border_window_size (Gtk::TEXT_WINDOW_RIGHT);
}

int View::get_borders_height () const
{
    return get_border_window_size (Gtk::TEXT_WINDOW_TOP)
            + get_border_window_size (Gtk::TEXT_WINDOW_BOTTOM);
}

void View::on_size_allocate (Gtk::Allocation &allocation)
{
    Gtk::TextView::on_size_allocate (allocation);
    // Size requests and allocations appear not to include margins
    int borders_width, borders_height;
    get_borders_size (borders_width, borders_height);
    g_debug ("view size allocate: borders size %dx%d",
            borders_width, borders_height);
    columns_ = (allocation.get_width () - borders_width) / cell_width_px_;
    rows_ = (allocation.get_height () - borders_height) / cell_height_px_;

    g_debug ("view allocation %dx%d, grid size %dx%d, buf %d",
            allocation.get_width (), allocation.get_height (),
            columns_, rows_, buffer_ != nullptr);

    // Does nothing if size hasn't changed
    if (buffer_ && buffer_->resize (columns_, rows_))
    {
        buffer_->get_nvim_bridge ().nvim_ui_try_resize (columns_, rows_);
    }
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

    g_debug ("Cell size %dx%d", cell_width_px_, cell_height_px_);
}

void View::get_preferred_width_vfunc (int &minimum, int &natural) const
{
    auto bw = get_borders_width ();
    g_debug ("bw %d", bw);
    minimum = 5 * cell_width_px_ + bw;
    natural = (buffer_ ? buffer_->get_columns () : 80) * cell_width_px_ + bw;
    g_debug ("View preferred width (buf %d) %d, %d", buffer_ != nullptr,
            minimum, natural);
}

void View::get_preferred_height_vfunc (int &minimum, int &natural) const
{
    auto bh = get_borders_height ();
    g_debug ("bh %d", bh);
    minimum = 5 * cell_height_px_ + bh;
    natural = (buffer_ ? buffer_->get_rows () : 30) * cell_height_px_ + bh;
    g_debug ("View preferred height (buf %d) %d, %d", buffer_ != nullptr,
            minimum, natural);
}

void View::on_redraw_mode_change (const std::string &mode)
{
    // Kludge to set cursor shape
    set_overwrite (mode == "normal");
}

void View::on_redraw_bell ()
{
    get_window (Gtk::TEXT_WINDOW_TEXT)->beep ();
}

void View::on_redraw_resize (int columns, int rows)
{
    if (buffer_->resize (columns, rows))
        queue_resize ();
}

}
