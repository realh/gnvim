/* nvim-grid-view.cpp
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

#include "app.h"
#include "nvim-grid-widget.h"

namespace Gnvim
{

enum FontSource
{
    FONT_SOURCE_GTK = 0,
    FONT_SOURCE_SYS,
    FONT_SOURCE_PREFS,
};

NvimGridWidget::NvimGridWidget(NvimGridView *view) : TextGridWidget(view)
{
    add_events(Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON2_MOTION_MASK |
            Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
            Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK |
            Gdk::FOCUS_CHANGE_MASK |
            Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK);
    set_can_focus();
    grab_focus();

    auto sc = get_style_context();
    if (!sc->has_class("gnvim"))
        sc->add_class("gnvim");
}

void NvimGridWidget::on_realize()
{
    TextGridWidget::on_realize();
    auto view = static_cast<NvimGridView *>(view_);
    if (!view->get_surface())
    {
        view->update_font(get_pango_context(), true);
        view->create_cairo_surface(this);
    }
}

void NvimGridWidget::on_size_allocate(Gtk::Allocation &alloc)
{
    TextGridWidget::on_size_allocate(alloc);
    view_->on_size_allocate(alloc);
}

bool NvimGridWidget::on_key_press_event(GdkEventKey *event)
{
    return get_nvim_grid_view()->on_key_press_event(event);
}

bool NvimGridWidget::on_button_press_event(GdkEventButton *event)
{
    return on_mouse_event(event->type, event->button,
            event->state, event->x, event->y);
}

bool NvimGridWidget::on_button_release_event(GdkEventButton *event)
{
    return on_mouse_event(event->type, event->button,
            event->state, event->x, event->y);
}

bool NvimGridWidget::on_motion_notify_event(GdkEventMotion *event)
{
    return get_nvim_grid_view()->on_motion_notify_event(event);
}

bool NvimGridWidget::on_scroll_event(GdkEventScroll *event)
{
    return get_nvim_grid_view()->on_scroll_event(event);
}

bool NvimGridWidget::on_focus_in_event(GdkEventFocus *e)
{
    get_nvim_grid_view()->on_focus_in_event();
    return TextGridWidget::on_focus_in_event(e);
}

bool NvimGridWidget::on_focus_out_event(GdkEventFocus *e)
{
    get_nvim_grid_view()->on_focus_out_event();
    return TextGridWidget::on_focus_out_event(e);
}

}
