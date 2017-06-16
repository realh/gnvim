/* nvim-grid-view.h
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

#ifndef GNVIM_NVIM_GRID_WIDGET_H
#define GNVIM_NVIM_GRID_WIDGET_H

#include "defns.h"

#include "nvim-grid-view.h"
#include "text-grid-widget.h"

namespace Gnvim
{

// TextGridWidget specialised for neovim
class NvimGridWidget : public TextGridWidget {
public:
    NvimGridWidget(NvimGridView *view);
protected:
    virtual void on_realize() override;

    virtual void on_size_allocate(Gtk::Allocation &) override;

    virtual bool on_key_press_event(GdkEventKey *) override;

    virtual bool on_button_press_event(GdkEventButton *) override;

    virtual bool on_button_release_event(GdkEventButton *) override;

    virtual bool on_motion_notify_event(GdkEventMotion *) override;

    virtual bool on_scroll_event(GdkEventScroll *) override;

    virtual bool on_focus_in_event(GdkEventFocus *) override;

    virtual bool on_focus_out_event(GdkEventFocus *) override;
private:
    NvimGridView *get_nvim_grid_view()
    {
        return static_cast<NvimGridView *>(view_);
    }

    std::shared_ptr<NvimBridge> get_nvim_bridge()
    {
        return get_nvim_grid_view()->get_nvim_bridge();
    }

    bool on_mouse_event(GdkEventType t, int button,
            guint modifiers, int x, int y)
    {
        return get_nvim_grid_view()->on_mouse_event(t, button, modifiers, x, y);
    }

    void do_scroll(const std::string &direction, int state)
    {
        get_nvim_grid_view()->do_scroll(direction, state);
    }
};

}

#endif // GNVIM_NVIM_GRID_VIEW_H
