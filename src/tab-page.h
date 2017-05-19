/* tab-page.h
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

#ifndef GNVIM_TAB_PAGE_H
#define GNVIM_TAB_PAGE_H

#include "defns.h"

#include <memory>

#include "bufsandtabs.h"

namespace Gnvim
{

/** Dummy widget for content of a tab page with an associated label.
 *  The real content is displayed by letting nvim change the text in the
 *  window's single GridView, but Gtk::Notebook expects each tab to contain
 *  a different widget, so this provides one that's invisible. It also provides
 *  convenient access to the label.
 */
class TabPage : public Gtk::Grid {
public:
    TabPage(const TabInfo &t);

    Gtk::Widget &get_label_widget()
    {
        return text_label_;
    }
private:
    Gtk::Label text_label_;
};

}

#endif // GNVIM_TAB_PAGE_H
