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

#include "app.h"
#include "nvim-bridge.h"
#include "view.h"

namespace Gnvim
{

enum FontSource
{
    FONT_SOURCE_GTK = 0,
    FONT_SOURCE_SYS,
    FONT_SOURCE_PREFS,
};

View::View () : buffer_ (nullptr)
{
    // Disabling wrapping should minimise nasty things happening during
    // asynchronous size changes
    set_wrap_mode (Gtk::WRAP_NONE);
    set_monospace ();
    default_font_ = get_pango_context ()->get_font_description ().to_string ();

    auto app_settings = Application::app_gsettings ();
    app_settings->signal_changed ("font").connect
            (sigc::mem_fun (this, &View::on_font_name_changed));
    app_settings->signal_changed ("font-source").connect
            (sigc::mem_fun (this, &View::on_font_source_changed));
    auto sys_settings = Application::sys_gsettings ();
    sys_settings->signal_changed ("monospace-font-name").connect
            (sigc::mem_fun (this, &View::on_font_name_changed));

    auto sc = get_style_context ();
    if (!sc->has_class ("gnvim"))
        sc->add_class ("gnvim");

    update_font (true);
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

void View::set_font (const Glib::ustring &desc, bool q_resize)
{
    if (!font_style_provider_)
    {
        font_style_provider_ = Gtk::CssProvider::create ();
        get_style_context ()->add_provider
            (font_style_provider_, GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
    auto sep = desc.find_last_of (' ');
    if (sep == Glib::ustring::npos)
    {
        g_critical (_("Invalid font description '%s'"), desc.c_str ());
        return;
    }
    try
    {
        Glib::ustring fsize (desc.substr (sep + 1));
        if (g_unichar_isdigit (desc.at (desc.size () - 1)))
        {
            fsize += "pt";
        }
        Glib::ustring family = desc.substr (0, sep);
        if (family.find_first_of (' ') != Glib::ustring::npos)
        {
            family = Glib::ustring (1, '"') + family + '"';
        }
        auto css = Glib::ustring ("textview.gnvim { font: ")
            + fsize + " " + family + "; }";
        font_style_provider_->load_from_data (css);
        // Changing the font in the widget's Pango context seems to have no
        // effect, which is why we're using a style provider, but setting the
        // Pango context too makes it easier for calculate_metrics () to get
        // the description back.
        get_pango_context ()->set_font_description (Pango::FontDescription
                (desc.size () ? desc : default_font_));
        calculate_metrics ();
        if (q_resize)
            resize_window ();
    }
    catch (const std::exception &e)
    {
        g_critical (_("std::exception setting font '%s': %s"),
                    desc.c_str (), e.what ());
    }
    catch (const Glib::Exception &e)
    {
        g_critical (_("Glib::Exception setting font '%s': %s"),
                    desc.c_str (), e.what ().c_str ());
    }
}

void View::resize_window ()
{
    auto view_alloc = get_allocation ();
    int ignored, nat_width, nat_height;
    get_preferred_width_vfunc (ignored, nat_width);
    get_preferred_height_vfunc (ignored, nat_height);
    bool changed = nat_width != view_alloc.get_width ()
                || nat_height != view_alloc.get_height ();
    auto win = static_cast<Gtk::Window *> (get_toplevel ());
    if (changed)
    {
        // I wasn't sure whether the queue_resize and set_size_request were
        // necessary. Things seem to work correctly without them, whereas with
        // them, the window initially resizes correctly, but in Wayland (and
        // in X11 with CSD?) if the size change is a reduction in width it
        // gains some of it back the next time the window is focused. Weird.

        //queue_resize ();
        if (win)
        {
            int w = nat_width + toplevel_width_ - view_alloc.get_width ();
            int h = nat_height + toplevel_height_ - view_alloc.get_height ();
            //win->set_size_request (w, h);
            win->resize (w, h);
        }
    }
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
    columns_ = (allocation.get_width () - borders_width) / cell_width_px_;
    rows_ = (allocation.get_height () - borders_height) / cell_height_px_;

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
}

void View::get_preferred_width_vfunc (int &minimum, int &natural) const
{
    auto bw = get_borders_width ();
    minimum = 5 * cell_width_px_ + bw;
    natural = (buffer_ ? buffer_->get_columns () : 80) * cell_width_px_ + bw;
}

void View::get_preferred_height_vfunc (int &minimum, int &natural) const
{
    auto bh = get_borders_height ();
    minimum = 5 * cell_height_px_ + bh;
    natural = (buffer_ ? buffer_->get_rows () : 30) * cell_height_px_ + bh;
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
        resize_window ();
}

void View::on_font_name_changed (const Glib::ustring &key)
{
    auto app_settings = Application::app_gsettings ();
    if ((key == "font"
            && app_settings->get_enum ("font-source") == FONT_SOURCE_PREFS)
        || (key == "monospace-font-name"
            && app_settings->get_enum ("font-source") == FONT_SOURCE_SYS))
    {
        update_font ();
    }
}

void View::on_font_source_changed (const Glib::ustring &)
{
    update_font ();
}

void View::update_font (bool init)
{
    auto app_settings = Application::app_gsettings ();
    switch (app_settings->get_enum ("font-source"))
    {
        case FONT_SOURCE_SYS:
            set_font (Application::sys_gsettings ()->get_string
                    ("monospace-font-name"), !init);
            break;
        case FONT_SOURCE_PREFS:
            set_font (app_settings->get_string ("font"), !init);
            break;
        default:    // FONT_SOURCE_GTK
            if (!init)
                set_font (default_font_);
            else
                calculate_metrics ();
            break;
    }
}

void View::on_parent_changed (Gtk::Widget *old_parent)
{
    Gtk::TextView::on_parent_changed (old_parent);
    // The width and height sent in Window::size-allocate signals are no use
    // because of CSD and Wayland issues, so we're supposed to use
    // gtk_window_get_size. But that should only be used during certain signal
    // handlers, size-allocate being one of them.
    if (old_parent && toplevel_size_allocate_connection_)
        toplevel_size_allocate_connection_.disconnect ();
    toplevel_size_allocate_connection_
        = get_toplevel ()->signal_size_allocate ().connect
            (sigc::mem_fun (this, &View::on_toplevel_size_allocate));
}

void View::on_toplevel_size_allocate (Gtk::Allocation &)
{
    static_cast<Gtk::Window *> (get_toplevel ())->get_size
        (toplevel_width_, toplevel_height_);
}

}
