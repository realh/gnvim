
/* app.h
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

#ifndef GNVIM_APP_H
#define GNVIM_APP_H

#include "defns.h"

namespace Gnvim
{

class Application : public Gtk::Application {
protected:
    Application();
public:
    static RefPtr<Application> create()
    {
        return RefPtr<Application> (new Application());
    }

    virtual int on_command_line(const RefPtr<Gio::ApplicationCommandLine> &)
            override;

    bool open_window(const RefPtr<Gio::ApplicationCommandLine> &);

    static RefPtr<Gio::Settings> app_gsettings();

    // "org.gnome.desktop.interface" for reading "monospace-font-name"
    static RefPtr<Gio::Settings> sys_gsettings();

    Glib::PropertyProxy<bool> property_dark_theme()
    {
        return prop_dark_theme_.get_proxy();
    }
protected:
    virtual void on_startup() override;

    virtual void on_window_removed(Gtk::Window *win) override;
private:
    bool on_opt_geometry(const Glib::ustring &, const Glib::ustring &, bool);

    void on_prop_dark_theme_changed();

    void on_action_about();
    void on_action_quit();

    Glib::OptionContext options_;
    Glib::OptionGroup main_opt_group_;
    bool opt_max_;
    int opt_width_, opt_height_;
    bool opt_help_nvim_;

    static RefPtr<Gio::Settings> app_gsettings_;
    static RefPtr<Gio::Settings> sys_gsettings_;
    Glib::Property<bool> prop_dark_theme_;
};

}

#endif // GNVIM_APP_H
