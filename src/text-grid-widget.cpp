/* text-grid-view.cpp
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

#include "defns.h"

#include "app.h"
#include "text-grid-widget.h"

namespace Gnvim
{

TextGridWidget::TextGridWidget(TextGridView *view) : view_(view)
{
}

void TextGridWidget::resize_window()
{
    auto view_alloc = get_allocation();
    int _, nat_width, nat_height;
    get_preferred_width_vfunc(_, nat_width);
    get_preferred_height_vfunc(_, nat_height);
    bool changed = nat_width != view_alloc.get_width()
                || nat_height != view_alloc.get_height();
    auto win = static_cast<Gtk::Window *> (get_toplevel());
    if (changed)
    {
        // I wasn't sure whether the queue_resize and set_size_request were
        // necessary. Things seem to work correctly without them, whereas with
        // them, the window initially resizes correctly, but in Wayland(and
        // in X11 with CSD?) if the size change is a reduction in width it
        // gains some of it back the next time the window is focused. Weird.

        //queue_resize();
        if (win)
        {
            int w = nat_width + toplevel_width_ - view_alloc.get_width();
            int h = nat_height + toplevel_height_ - view_alloc.get_height();
            //win->set_size_request(w, h);
            win->resize(w, h);
        }
    }
}

void TextGridWidget::on_parent_changed(Gtk::Widget *old_parent)
{
    Gtk::DrawingArea::on_parent_changed(old_parent);
    // The width and height sent in Window::size-allocate signals are no use
    // because of CSD and Wayland issues, so we're supposed to use
    // gtk_window_get_size. But that should only be used during certain signal
    // handlers, size-allocate being one of them.
    if (old_parent && toplevel_size_allocate_connection_)
        toplevel_size_allocate_connection_.disconnect();
    auto win = get_toplevel();
    if (win)
    {
        toplevel_size_allocate_connection_
            = win->signal_size_allocate().connect
                (sigc::mem_fun(this, &TextGridWidget::on_toplevel_size_allocate));
    }
}

void TextGridWidget::on_size_allocate(Gtk::Allocation &allocation)
{
    Gtk::DrawingArea::on_size_allocate(allocation);
    if (view_)
        view_->on_size_allocate(allocation);
}

void TextGridWidget::get_preferred_width_vfunc(int &minimum, int &natural) const
{
    if (view_)
    {
        view_->get_preferred_width(minimum, natural);
    }
    else
    {
        minimum = 5 * 8;
        natural = 80 * 8;
    }
}

void TextGridWidget::get_preferred_height_vfunc(int &minimum, int &natural) const
{
    if (view_)
    {
        view_->get_preferred_height(minimum, natural);
    }
    else
    {
        minimum = 5 * 16;
        natural = 30 * 16;
    }
}

void TextGridWidget::on_toplevel_size_allocate(Gtk::Allocation &)
{
    static_cast<Gtk::Window *> (get_toplevel())->get_size
        (toplevel_width_, toplevel_height_);
}

bool TextGridWidget::on_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    if (!view_)
    {
        g_warning("Trying to draw TextGridWidget with no view");
        return Gtk::DrawingArea::on_draw(cr);
    }
    auto alloc = get_allocation();
    int w = alloc.get_width();
    int h = alloc.get_height();
    auto surf = view_->get_surface();
    cr->save();
    cr->rectangle(0, 0, w, h);
    cr->clip();
    view_->fill_background_px(cr, 0, 0, w, h);
    //double l, t, r, b;
    //cr->get_clip_extents(l, t, r, b);
    //g_debug("redraw clip extents L:%f T:%f R:%f B:%f", l, t, r, b);
    cr->set_source(view_->get_surface(), 0, 0);
    cr->paint();
    view_->draw_cursor(cr);
    cr->restore();
    return true;
}

bool TextGridWidget::on_focus_in_event(GdkEventFocus *e)
{
    if (view_)
        view_->show_cursor();
    return Gtk::DrawingArea::on_focus_in_event(e);
}

bool TextGridWidget::on_focus_out_event(GdkEventFocus *e)
{
    if (view_)
        view_->show_cursor();
    return Gtk::DrawingArea::on_focus_out_event(e);
}

}
