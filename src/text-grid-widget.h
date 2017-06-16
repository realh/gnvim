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

#ifndef GNVIM_TEXT_GRID_WIDGET_H
#define GNVIM_TEXT_GRID_WIDGET_H

#include "defns.h"

#include <utility>
#include <vector>

#include "text-grid-view.h"

namespace Gnvim
{

/// Displays the TextGridView in a widget
class TextGridWidget : public Gtk::DrawingArea {
public:
    TextGridWidget(TextGridView *view);

    virtual ~TextGridWidget() = default;

    // Try to resize the parent window to fit the desired number of columns
    // and lines
    void resize_window();
protected:
    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

    virtual void on_parent_changed(Gtk::Widget *old_parent) override;

    virtual void on_size_allocate(Gtk::Allocation &) override;

    virtual bool on_focus_in_event(GdkEventFocus *) override;

    virtual bool on_focus_out_event(GdkEventFocus *) override;

    virtual void get_preferred_width_vfunc(int &minimum, int &natural)
            const override;

    virtual void get_preferred_height_vfunc(int &minimum, int &natural)
            const override;

    void on_toplevel_size_allocate(Gtk::Allocation &);

    TextGridView *view_;

    int cell_width_px_, cell_height_px_;
    int columns_, lines_;

    sigc::connection toplevel_size_allocate_connection_;
    int toplevel_width_ {0}, toplevel_height_ {0};
};

}

#endif // GNVIM_TEXT_GRID_WIDGET_H
