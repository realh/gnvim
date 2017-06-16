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

#ifndef GNVIM_NVIM_GRID_VIEW_H
#define GNVIM_NVIM_GRID_VIEW_H

#include "defns.h"

#include <memory>

#include <msgpack.hpp>

#include "nvim-bridge.h"
#include "text-grid-view.h"

namespace Gnvim
{

class NvimGridWidget;

// TextGridView specialised for neovim
class NvimGridView : public TextGridView {
public:
    NvimGridView(std::shared_ptr<NvimBridge> nvim, int columns, int lines,
            const std::string &font_name = "");

    virtual void on_size_allocate(Gtk::Allocation &) override;

    bool on_key_press_event(GdkEventKey *);

    bool on_button_press_event(GdkEventButton *);

    bool on_button_release_event(GdkEventButton *);

    bool on_motion_notify_event(GdkEventMotion *);

    bool on_scroll_event(GdkEventScroll *);

    void on_focus_in_event();

    void on_focus_out_event();

    void update_font(bool init = false);

    std::shared_ptr<NvimBridge> get_nvim_bridge()
    {
        return nvim_;
    }

    bool on_mouse_event(GdkEventType, int button,
            guint modifiers, int x, int y);

    void do_scroll(const std::string &direction, int state);
private:
    void update_redraw_region(int left, int top, int right, int bottom);

    std::string region_to_string();

    void on_redraw_start();
    void on_redraw_mode_change(const std::string &mode);
    void on_redraw_resize(int columns, int lines);
    void on_redraw_bell();
    void on_redraw_update_fg(int colour);
    void on_redraw_update_bg(int colour);
    void on_redraw_update_sp(int colour);
    void on_redraw_cursor_goto(int line, int col);
    void on_redraw_put(const msgpack::object_array &);
    void on_redraw_clear();
    void on_redraw_eol_clear();
    void on_redraw_highlight_set(const msgpack::object &map_o);
    void on_redraw_set_scroll_region(int top, int bot, int left, int right);
    void on_redraw_scroll(int count);
    void on_redraw_end();

    /* I don't think nvim sets the scroll region before scrolling the entire
     * screen so we need to initialise it; probably best called whenever the 
     * viewport size changes.
     */
    void reset_scroll_region();

    // Used for both app and sys fonts, using key to work out which
    void on_font_name_changed(const Glib::ustring &key);

    void on_font_source_changed(const Glib::ustring &key);

    /* These are called with an empty key during init to signify that they're
     * being used to retrieve init values and the view isn't necessarily ready
     * to show the cursor yet.
     */
    void on_cursor_width_changed(const Glib::ustring &key);
    void on_cursor_fg_changed(const Glib::ustring &key);
    void on_cursor_bg_changed(const Glib::ustring &key);
    void on_sync_shada_changed(const Glib::ustring &key);

    /* If we've asked nvim to resize in response to size-allocate from GUI we
     * shouldn't then try to resize the GUI in response to nvim's redraw_resize
     * event. In particular, Wayland or CSD causes a spurious undersized
     * size-allocate when unmaximising, before sending the correct allocation.
     * If we call Window::resize in response to that, the window ends up at the
     * wrong size. This counter keeps track of which resize events were
     * GUI-driven.
     */
    int gui_resize_counter_ {0};

    std::shared_ptr<NvimBridge> nvim_;

    // redraw limits are inclusive
    struct {
        int left, top, right, bottom;
    } redraw_region_, scroll_region_;

    int cursor_rdrw_x_, cursor_rdrw_y_;

    CellAttributes current_attrs_;

    unsigned beam_cursor_width_;

    bool sync_shada_;
};

}

#endif // GNVIM_NVIM_GRID_VIEW_H
