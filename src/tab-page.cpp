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
    box_.pack_end(close_button_, false, false);
}

void TabPage::on_close_button_clicked()
{
    std::ostringstream s;
    // This assumes GUI tab order is in sync with nvim, which is reasonably safe
    s << (notebook_->child_property_position(*this).get_value() + 1)
        << "tabclose";
    get_nvim_bridge()->nvim_command(s.str());
}

}
