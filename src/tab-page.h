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
#include "nvim-grid-widget.h"

namespace Gnvim
{

// NvimGridWidget associated with a tab page
class TabPage : public NvimGridWidget {
public:
    TabPage(Gtk::Notebook *notebook, NvimGridView *view, const TabInfo &t)
        : TabPage(notebook, view, t.handle)
    {}

    TabPage(Gtk::Notebook *notebook, NvimGridView *view, const VimTabpage &t);

    Gtk::Widget &get_label_widget()
    {
        return box_;
    }

    void set_label_text(const std::string &s)
    {
        text_label_.set_text(s);
    }

    std::string get_label_text()
    {
        return text_label_.get_text();
    }

    const VimTabpage &get_vim_handle() const
    {
        return handle_;
    }
private:
    void on_close_button_clicked();

    bool on_close_button_pressed(GdkEventButton *);

    void send_tab_command(const char *cmd, bool force = false);

    Gtk::Notebook *notebook_;
    Gtk::Box box_;
    Gtk::Label text_label_;
    Gtk::Button close_button_;
    VimTabpage handle_;
};

}

#endif // GNVIM_TAB_PAGE_H
