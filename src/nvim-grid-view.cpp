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
#include "nvim-grid-view.h"

namespace Gnvim
{

enum FontSource
{
    FONT_SOURCE_GTK = 0,
    FONT_SOURCE_SYS,
    FONT_SOURCE_PREFS,
};

NvimGridView ::NvimGridView(std::shared_ptr<NvimBridge> nvim,
        int columns, int lines,
        Gtk::Widget *widget,
        const std::string &font_name)
: TextGridView(columns, lines, font_name), nvim_(nvim)
{
    set_current_widget(widget);

    auto app_settings = Application::app_gsettings();
    app_settings->signal_changed("font").connect
            (sigc::mem_fun(this, &NvimGridView::on_font_name_changed));
    app_settings->signal_changed("font-source").connect
            (sigc::mem_fun(this, &NvimGridView::on_font_source_changed));
    app_settings->signal_changed("cursor-width").connect
            (sigc::mem_fun(this, &NvimGridView::on_cursor_width_changed));
    on_cursor_width_changed("");
    app_settings->signal_changed("cursor-bg").connect
            (sigc::mem_fun(this, &NvimGridView::on_cursor_bg_changed));
    on_cursor_bg_changed("");
    app_settings->signal_changed("cursor-fg").connect
            (sigc::mem_fun(this, &NvimGridView::on_cursor_fg_changed));
    on_cursor_fg_changed("");
    app_settings->signal_changed("sync-shada").connect
            (sigc::mem_fun(this, &NvimGridView::on_sync_shada_changed));
    on_sync_shada_changed("");

    auto sys_settings = Application::sys_gsettings();
    sys_settings->signal_changed("monospace-font-name").connect
            (sigc::mem_fun(this, &NvimGridView::on_font_name_changed));

    update_font(true);
    on_redraw_mode_change("normal");

    reset_scroll_region();

    nvim_->redraw_start.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_start));
    nvim_->redraw_mode_change.connect
        (sigc::mem_fun(this, &NvimGridView::on_redraw_mode_change));
    nvim_->redraw_bell.connect
        (sigc::mem_fun(this, &NvimGridView::on_redraw_bell));
    nvim_->redraw_visual_bell.connect
        (sigc::mem_fun(this, &NvimGridView::on_redraw_bell));
    nvim_->redraw_resize.connect
        (sigc::mem_fun(this, &NvimGridView::on_redraw_resize));
    nvim_->redraw_update_bg.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_update_bg));
    nvim_->redraw_update_fg.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_update_fg));
    nvim_->redraw_update_sp.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_update_sp));
    nvim_->redraw_cursor_goto.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_cursor_goto));
    nvim_->redraw_put.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_put));
    nvim_->redraw_clear.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_clear));
    nvim_->redraw_eol_clear.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_eol_clear));
    nvim_->redraw_highlight_set.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_highlight_set));
    nvim_->redraw_set_scroll_region.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_set_scroll_region));
    nvim_->redraw_scroll.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_scroll));
    nvim_->redraw_end.connect
            (sigc::mem_fun(this, &NvimGridView::on_redraw_end));
}

void NvimGridView::on_size_allocate(Gtk::Allocation &alloc)
{
    int old_cols = columns_;
    int old_lines = lines_;
    TextGridView::on_size_allocate(alloc);
    if (old_cols != columns_ || old_lines != lines_)
    {
        ++gui_resize_counter_;
        nvim_->nvim_ui_try_resize(columns_, lines_);
    }
}

Glib::ustring modifier_string(guint state)
{
    std::string s;
    if (state & GDK_SUPER_MASK)
    {
        s = "D-";
    }
    if (state & GDK_MOD1_MASK)
    {
        s = "A-";
    }
    if (state & GDK_CONTROL_MASK)
        s += "C-";
    if (state & GDK_SHIFT_MASK)
        s += "S-";
    return s;
}

bool NvimGridView::on_key_press_event(GdkEventKey *event)
{
    if (event->is_modifier)
    {
        return true;
    }

    bool special = false;
    gunichar uk = gdk_keyval_to_unicode(event->keyval);
    Glib::ustring s = gdk_keyval_name(event->keyval);

    bool kp = s.find("KP_") == 0;
    if (kp)
    {
        special = true;
        s = s.substr(3);
        if (s == "Add")
            s = "Plus";
        else if (s == "Subtract")
            s = "Minus";
        else if (s == "Decimal")
            s = "Point";
        s = Glib::ustring(1, 'k') + s;
    }

    if (g_unichar_isprint(uk))
    {
        s = Glib::ustring(1, uk);
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

    /*
    g_debug("state %x, super %x, meta %x, mod1 %x, mod2 %x",
            event->state,
            GDK_SUPER_MASK, GDK_META_MASK, GDK_MOD1_MASK, GDK_MOD2_MASK);
    if (event->state & GDK_SUPER_MASK)
    {
        g_debug("Super");
    }
    if (event->state & GDK_META_MASK)
    {
        g_debug("Meta");
    }
    */
    if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
    {
        special = true;
        s = modifier_string(event->state) + s;
    }

    if (special)
        s = Glib::ustring(1, '<') + s + '>';

    //g_debug("Keypress %s", s.c_str());

    nvim_->nvim_input(s);
    return true;
}

bool NvimGridView::on_button_press_event(GdkEventButton *event)
{
    return on_mouse_event(event->type, event->button,
            event->state, event->x, event->y);
}

bool NvimGridView::on_button_release_event(GdkEventButton *event)
{
    return on_mouse_event(event->type, event->button,
            event->state, event->x, event->y);
}

bool NvimGridView::on_motion_notify_event(GdkEventMotion *event)
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
    return on_mouse_event(event->type, b, event->state, event->x, event->y);
}

bool NvimGridView::on_scroll_event(GdkEventScroll *event)
{
    switch(event->direction)
    {
        case GDK_SCROLL_UP:
            do_scroll("Up", event->state);
            break;
        case GDK_SCROLL_DOWN:
            do_scroll("Down", event->state);
            break;
        case GDK_SCROLL_LEFT:
            do_scroll("Left", event->state);
            break;
        case GDK_SCROLL_RIGHT:
            do_scroll("Right", event->state);
            break;
        case GDK_SCROLL_SMOOTH:
            // Bit of a kludge, vim doesn't currently have a good way to do
            // smooth scrolling
            if (event->delta_x < 0)
                do_scroll("Left", event->state);
            else if (event->delta_x > 0)
                do_scroll("Right", event->state);
            if (event->delta_y < 0)
                do_scroll("Up", event->state);
            else if (event->delta_y > 0)
                do_scroll("Down", event->state);
            break;
    }
    return true;
}

static const char *mouse_button_name(guint button)
{
    switch(button)
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

bool NvimGridView::on_mouse_event(GdkEventType etype, int button,
        guint modifiers, int x, int y)
{
    if (!button || button >= 4)
        return true;

    Glib::ustring etype_str;
    switch(etype)
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
        but_str = mouse_button_name(button);

    convert_coords_to_cells(x, y);

    char *inp = g_strdup_printf("<%s%s%s><%d,%d>",
            modifier_string(modifiers).c_str(),
            but_str.c_str(), etype_str.c_str(),
            x, y);
    //g_debug("Mouse event %s", inp);
    nvim_->nvim_input(inp);
    g_free(inp);

    return true;
}

std::string NvimGridView::region_to_string()
{
    std::ostringstream s;
    s << "L:" << redraw_region_.left << " T:" << redraw_region_.top
        << " R:" << redraw_region_.right << " B:" << redraw_region_.bottom;
    return s.str();
}

void NvimGridView::on_redraw_start()
{
    cursor_rdrw_x_ = cursor_col_;
    cursor_rdrw_y_ = cursor_line_;
    redraw_region_.left = columns_;
    redraw_region_.top = lines_;
    redraw_region_.right = 0;
    redraw_region_.bottom = 0;
    /*
    g_debug("on_redraw_start, resetting region to %s",
            region_to_string().c_str());
    */
}

// TODO
/*
void NvimGridView::set_current_widget(Gtk::Widget *w)
{
    current_widget_ = static_cast<NvimGridWidget *>(w);
}
*/

void NvimGridView::on_redraw_mode_change(const std::string &mode)
{
    if (mode == "insert")
        cursor_width_ = beam_cursor_width_;
    else
        cursor_width_ = 0;
    // TODO
    /*
    if (current_widget_)
        current_widget_->show_cursor();
    */
}

void NvimGridView::on_redraw_resize(int columns, int lines)
{
    if (columns != columns_ || lines != lines_)
    {
        redraw_region_.left = 0;
        redraw_region_.top = 0;
        redraw_region_.right = columns_ - 1;
        redraw_region_.bottom = lines_ - 1;
        /*
        g_debug("on_redraw_resize, changing region to %s",
                region_to_string().c_str());
        */
        columns_ = columns;
        lines_ = lines;
        grid_.resize(columns, lines);
        reset_scroll_region();
        if (gui_resize_counter_)
            --gui_resize_counter_;
        // TODO
        /*
        else if (current_widget_)
            current_widget_->resize_window();
        */
    }
}

void NvimGridView::on_redraw_bell()
{
    // TODO
    /*
    if (current_widget_)
        current_widget_->get_window()->beep();
    */
}

void NvimGridView::on_redraw_update_fg(int colour)
{
    grid_.get_default_attributes().set_foreground_rgb(colour);
    if (!colour_cursor_)
        cursor_attr_.set_background_rgb(colour);
    global_redraw_pending_ = true;
}

void NvimGridView::on_redraw_update_bg(int colour)
{
    grid_.get_default_attributes().set_background_rgb(colour);
    if (!colour_cursor_)
        cursor_attr_.set_foreground_rgb(colour);
    global_redraw_pending_ = true;
}

void NvimGridView::on_redraw_update_sp(int colour)
{
    grid_.get_default_attributes().set_special_rgb(colour);
    global_redraw_pending_ = true;
}

void NvimGridView::on_redraw_cursor_goto(int line, int col)
{
    //g_debug("cursor_goto %d, %d", col, line);
    // Need to make sure the cursor's old position gets redrawn
    update_redraw_region(cursor_rdrw_x_, cursor_rdrw_y_,
            cursor_rdrw_x_, cursor_rdrw_y_);
    cursor_rdrw_x_ = col;
    cursor_rdrw_y_ = line;
    update_redraw_region(col, line, col, line);
}

void NvimGridView::on_redraw_put(const msgpack::object_array &text_ar)
{
    int start_col = cursor_rdrw_x_;
    for (gsize i = 1; i < text_ar.size; ++i)
    {
        const auto &o = text_ar.ptr[i];
        if (o.type == msgpack::type::STR)
        {
            const auto &ms = o.via.str;
            grid_.set_text_at(std::string(ms.ptr, ms.ptr + ms.size),
                    cursor_rdrw_x_++, cursor_rdrw_y_);
        }
        else if (o.type == msgpack::type::ARRAY)
        {
            const auto &ma = o.via.array;
            for (gsize j = 0; j < ma.size; ++j)
            {
                const auto &ms = ma.ptr[j].via.str;
                grid_.set_text_at(std::string(ms.ptr, ms.ptr + ms.size),
                        cursor_rdrw_x_++, cursor_rdrw_y_);
            }
        }
    }

    grid_.apply_attrs(current_attrs_,
            start_col, cursor_rdrw_y_, cursor_rdrw_x_ - 1, cursor_rdrw_y_);

    if (global_redraw_pending_)
        return;

    grid_.draw_line(grid_cr_, cursor_rdrw_y_, start_col, cursor_rdrw_x_ - 1);

    //g_debug("on_redraw_put:");
    update_redraw_region(start_col, cursor_rdrw_y_,
            cursor_rdrw_x_, cursor_rdrw_y_);
}

void NvimGridView::reset_scroll_region()
{
    scroll_region_.left = 0;
    scroll_region_.top = 0;
    scroll_region_.right = columns_ - 1;
    scroll_region_.bottom = lines_ - 1;
}

void NvimGridView::update_redraw_region
(int left, int top, int right, int bottom)
{
    if (left < redraw_region_.left)
        redraw_region_.left = left;
    if (right - 1 > redraw_region_.right)
        redraw_region_.right = right - 1;
    if (top < redraw_region_.top)
        redraw_region_.top = top;
    if (bottom > redraw_region_.bottom)
        redraw_region_.bottom = bottom;
    /*
    g_debug("update_redraw_region: merged L: %d T: %d R: %d B: %d to %s",
            left, top, right, bottom, region_to_string().c_str());
    */
}

void NvimGridView::on_redraw_clear()
{
    clear();
    redraw_region_.left = 0;
    redraw_region_.top = 0;
    redraw_region_.right = columns_ - 1;
    redraw_region_.bottom = lines_ - 1;
    /*
    g_debug("on_redraw_clear, setting region to %s",
            region_to_string().c_str());
    */
}

void NvimGridView::on_redraw_eol_clear()
{
    //g_debug("on_redraw_eol_clear:");
    update_redraw_region(cursor_rdrw_x_, cursor_rdrw_y_,
            columns_ - 1, cursor_rdrw_y_);
    clear(cursor_rdrw_x_, cursor_rdrw_y_, columns_ - 1, cursor_rdrw_y_);
}

void NvimGridView::on_redraw_highlight_set(const msgpack::object &map_o)
{
    if (map_o.type != msgpack::type::MAP)
    {
        g_critical("Got sent type %d as arg for highlight_set, expected MAP",
                map_o.type);
        return;
    }
    const auto &map_m = map_o.via.map;

    if (!map_m.size)
    {
        current_attrs_ = grid_.get_default_attributes();
        return;
    }

    current_attrs_.reset();

    int foreground = -1, background = -1, special = -1;
    bool reverse = false;
    bool italic = false;
    bool bold = false;
    bool underline = false;
    bool undercurl = false;

    for (guint n = 0; n < map_m.size; ++n)
    {
        const auto &kv = map_m.ptr[n];
        std::string k;
        kv.key.convert(k);
        if (k == "foreground")
            kv.val.convert(foreground);
        else if (k == "background")
            kv.val.convert(background);
        else if (k == "special")
            kv.val.convert(special);
        else if (k == "reverse")
            kv.val.convert(reverse);
        else if (k == "italic")
            kv.val.convert(italic);
        else if (k == "bold")
            kv.val.convert(bold);
        else if (k == "underline")
            kv.val.convert(underline);
        else if (k == "undercurl")
            kv.val.convert(undercurl);
    }

    if (foreground == -1)
        foreground = grid_.get_default_foreground_rgb();
    if (background == -1)
        background = grid_.get_default_background_rgb();
    if (special == -1)
        special = grid_.get_default_special_rgb();

    if (reverse)
    {
        current_attrs_.set_background_rgb(foreground);
        current_attrs_.set_foreground_rgb(background);
    }
    else
    {
        current_attrs_.set_background_rgb(background);
        current_attrs_.set_foreground_rgb(foreground);
    }
    current_attrs_.set_special_rgb(special);
    current_attrs_.set_bold(bold);
    current_attrs_.set_italic(italic);
    current_attrs_.set_underline(underline);
    current_attrs_.set_undercurl(undercurl);
}

void NvimGridView::on_redraw_set_scroll_region(int top, int bot,
        int left, int right)
{
    scroll_region_.left = left;
    scroll_region_.top = top;
    scroll_region_.right = right;
    scroll_region_.bottom = bot;
}

void NvimGridView::on_redraw_scroll(int count)
{
    scroll(scroll_region_.left, scroll_region_.top,
            scroll_region_.right, scroll_region_.bottom, count);
    //g_debug("on_redraw_scroll:");
    update_redraw_region(scroll_region_.left, scroll_region_.top,
            scroll_region_.right, scroll_region_.bottom);
}

void NvimGridView::on_redraw_end()
{
    /*
    g_debug("on_redraw_end queueing redraw of: %s",
            region_to_string(). c_str());
    */
    if (cursor_rdrw_x_ != cursor_col_ || cursor_rdrw_y_ != cursor_line_)
    {
        update_redraw_region(cursor_col_, cursor_line_,
                cursor_col_ + 1, cursor_line_ + 1);
        cursor_col_ = cursor_rdrw_x_;
        cursor_line_ = cursor_rdrw_y_;
        update_redraw_region(cursor_col_, cursor_line_,
                cursor_col_ + 1, cursor_line_ + 1);
        // TODO
        /*
        if (current_widget_)
            current_widget_->show_cursor();
        */
    }
    if (global_redraw_pending_)
    {
        global_redraw_pending_ = false;
        redraw_view();
        // TODO
        /*
        if (current_widget_)
            current_widget_->queue_draw();
        */
    }
    // TODO
    //else if (redraw_region_.left <= redraw_region_.right
    //        && redraw_region_.top <= redraw_region_.bottom
    //        && current_widget_)
    //{
    //    current_widget_->queue_draw_area(redraw_region_.left * cell_width_px_,
    //            redraw_region_.top * cell_height_px_,
    //            (redraw_region_.right - redraw_region_.left + 1)
    //                * cell_width_px_,
    //            (redraw_region_.bottom - redraw_region_.top + 1)
    //                * cell_height_px_);
    //    /*
    //    auto alloc = current_widget_->get_allocation();
    //    g_debug("qda %d,%d+%dx%d; alloc %dx%d",
    //            redraw_region_.left * cell_width_px_,
    //            redraw_region_.top * cell_height_px_,
    //            (redraw_region_.right - redraw_region_.left + 1)
    //                * cell_width_px_,
    //            (redraw_region_.bottom - redraw_region_.top + 1)
    //                * cell_height_px_,
    //            alloc.get_width(), alloc.get_height());
    //    */
    //}
}

void NvimGridView::do_scroll(const std::string &direction, int state)
{
    auto s = std::string(1, '<') + modifier_string(state)
        + "ScrollWheel" + direction + '>';
    nvim_->nvim_input(s);
}

// Used for both app and sys fonts, using key to work out which
void NvimGridView::on_font_name_changed(const Glib::ustring &key)
{
    auto app_settings = Application::app_gsettings();
    if ((key == "font"
            && app_settings->get_enum("font-source") == FONT_SOURCE_PREFS)
        || (key == "monospace-font-name"
            && app_settings->get_enum("font-source") == FONT_SOURCE_SYS))
    {
        update_font();
    }
}

void NvimGridView::on_font_source_changed(const Glib::ustring &)
{
    update_font();
}

// TODO
/*
void NvimGridView::update_font(bool init)
{
    if (!current_widget_)
    {
        g_critical("Tried to set font with no widget to provide Pango context");
        return;
    }
    auto app_settings = Application::app_gsettings();
    auto pc = current_widget->get_pango_context();
    switch(app_settings->get_enum("font-source"))
    {
        case FONT_SOURCE_SYS:
            set_font(pc, Application::sys_gsettings()->get_string
                    ("monospace-font-name"), !init);
            break;
        case FONT_SOURCE_PREFS:
            set_font(pc, app_settings->get_string("font"), !init);
            break;
        default:    // FONT_SOURCE_GTK
            set_font(pc, DEFAULT_FONT, !init);
            break;
    }
}
*/

void NvimGridView::on_cursor_width_changed(const Glib::ustring &key)
{
    beam_cursor_width_ =
        Application::app_gsettings()->get_uint("cursor-width");
    if (cursor_width_)
        cursor_width_ = beam_cursor_width_;
    // TODO
    (void) key;
    /*
    if (key.size() && current_widget_)
        current_widget_->show_cursor();
    */
}

void NvimGridView::on_cursor_bg_changed(const Glib::ustring &key)
{
    auto c = Application::app_gsettings()->get_string("cursor-bg");
    if (c.size())
    {
        cursor_attr_.set_background_rgb(CellAttributes::parse_colour(c));
        colour_cursor_ = true;
    }
    else
    {
        cursor_attr_.set_background_rgb(grid_.get_default_foreground_rgb());
        colour_cursor_ = false;
    }
    // TODO
    (void) key;
    /*
    if (key.size() && current_widget_)
        current_widget_->show_cursor();
    */
}

void NvimGridView::on_cursor_fg_changed(const Glib::ustring &key)
{
    auto c = Application::app_gsettings()->get_string("cursor-fg");
    if (c.size())
    {
        cursor_attr_.set_foreground_rgb(CellAttributes::parse_colour(c));
        colour_cursor_ = true;
    }
    else
    {
        cursor_attr_.set_foreground_rgb(grid_.get_default_background_rgb());
        colour_cursor_ = false;
    }
    // TODO
    (void) key;
    /*
    if (key.size() && current_widget_)
        current_widget_->show_cursor();
    */
}

void NvimGridView::on_sync_shada_changed(const Glib::ustring &)
{
    sync_shada_ = Application::app_gsettings()->get_boolean("sync-shada");
}

void NvimGridView::on_focus_in_event()
{
    nvim_->nvim_command("if exists('#FocusGained')|doauto FocusGained|endif");
    //nvim_->nvim_command("silent doauto FocusGained");
    if (sync_shada_)
        nvim_->nvim_command("rshada");
}

void NvimGridView::on_focus_out_event()
{
    nvim_->nvim_command("if exists('#FocusLost')|doauto FocusLost|endif");
    //nvim_->nvim_command("silent doauto FocusLost");
    if (sync_shada_)
        nvim_->nvim_command("wshada");
}

}
