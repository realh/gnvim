/* tab-page.cpp
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

#include <sstream>

#include "tab-page.h"

namespace Gnvim
{

TabPage::TabPage(Gtk::Notebook *notebook,
        NvimGridView *view, const VimTabpage &t)
    : NvimGridWidget(view), notebook_(notebook), text_label_(""), handle_(t)
{
    std::ostringstream s;
    s << "Tab " << gint64(t);
    text_label_.set_text(s.str());

    box_.pack_start(text_label_);

    close_button_.set_image_from_icon_name("window-close-symbolic",
            Gtk::ICON_SIZE_MENU);
    close_button_.set_relief(Gtk::RELIEF_NONE);
    close_button_.set_focus_on_click(false);
    close_button_.signal_clicked().connect
        (sigc::mem_fun(*this, &TabPage::on_close_button_clicked));
    close_button_.signal_button_press_event().connect
        (sigc::mem_fun(*this, &TabPage::on_close_button_pressed));
    box_.pack_end(close_button_, false, false);
}

// "clicked" signal is much easier than working out whether a press/release
// is a click, but it only works for button 1 and we need to get the modifiers
// from the source event
void TabPage::on_close_button_clicked()
{
    auto event = gtk_get_current_event();
    bool force = event && (event->button.state && Gdk::SHIFT_MASK);
    auto cmd = (event && (event->button.state && Gdk::CONTROL_MASK))
        ? "tabonly" : "tabclose";
    send_tab_command(cmd, force);
}

bool TabPage::on_close_button_pressed(GdkEventButton *event)
{
    bool force = event->state & Gdk::SHIFT_MASK;
    if (event->button == 3)
    {
        send_tab_command("tabonly", force);
        return true;
    }
    return false;
}

void TabPage::send_tab_command(const char *cmd, bool force)
{
    std::ostringstream s;
    // This assumes GUI tab order is in sync with nvim, which is reasonably safe
    s << (notebook_->child_property_position(*this).get_value() + 1) << cmd;
    if (force)
        s << '!';
    get_nvim_bridge()->nvim_command(s.str());
}

}
