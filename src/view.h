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
    View ();

    // Want to be passed an unref'd pointer because super takes a ref, but
    // it's a ref to Gtk::TextBuffer
    View (Buffer *buffer);

    void set_buffer (Buffer *buffer);

    bool has_buffer () const
    {
        return buffer_ != nullptr;
    }

    int get_allocated_columns () const
    {
        return columns_;
    }

    int get_allocated_rows () const
    {
        return rows_;
    }

    // desc is a pango font description string or "" for GTK default
    void set_font (const Glib::ustring &desc = "", bool q_resize = true);
protected:
    virtual void on_size_allocate (Gtk::Allocation &) override;

    virtual bool on_key_press_event (GdkEventKey *) override;

    virtual bool on_button_press_event (GdkEventButton *) override;

    virtual bool on_button_release_event (GdkEventButton *) override;

    virtual bool on_motion_notify_event (GdkEventMotion *) override;

    virtual bool on_scroll_event (GdkEventScroll *) override;

    virtual void get_preferred_width_vfunc (int &minimum, int &natural)
            const override;

    virtual void get_preferred_height_vfunc (int &minimum, int &natural)
            const override;
private:
    bool on_mouse_event (GdkEventType, int button,
            guint modifiers, int x, int y);

    void on_redraw_mode_change (const std::string &mode);

    void on_redraw_resize (int columns, int rows);

    void on_redraw_bell ();

    void calculate_metrics ();

    int get_borders_width () const;

    int get_borders_height () const;

    // Used for both app and sys fonts, using key to work out which
    void on_font_name_changed (const Glib::ustring &key);

    void on_font_source_changed (const Glib::ustring &key);

    // If init is true, this is being called at construction, in which case
    // don't set font if "font-source" pref is "gtk", and don't queue resize
    void update_font (bool init = false);

    // Try to resize the parent window to fit the desired number of columns
    // and lines
    void resize_window ();

    void get_borders_size (int &width, int &height) const
    {
        width = get_borders_width ();
        height = get_borders_height ();
    }

    // in = window coords eg from mouse event, out = vim text coords
    void convert_coords_to_cells (int &x, int &y)
    {
        x /= cell_width_px_;
        y /= cell_height_px_;
    }

    Buffer *buffer_;

    int cell_width_px_, cell_height_px_;
    int columns_ {-1}, rows_ {-1};

    Glib::ustring default_font_;
    RefPtr<Gtk::CssProvider> font_style_provider_;
};

}

#endif // GNVIM_VIEW_H
