/* app.cpp
 *
 * Copyright (C) 2017 Tony Houghton
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
#include "window.h"

namespace Gnvim
{

Application::Application ()
        : Gtk::Application ("uk.co.realh.gnvim",
            Gio::APPLICATION_HANDLES_COMMAND_LINE)
{
}

void Application::on_activate ()
{
    open_window ();
}

int Application::on_command_line (const RefPtr<Gio::ApplicationCommandLine> &cl)
{
    if (!open_window (cl))
    {
        g_warning ("Failed to open window, exiting this instance");
        return 1;
    }
    return 0;
}

bool Application::open_window (const RefPtr<Gio::ApplicationCommandLine> &cl)
{
    try {
        auto win = new Window (cl);
        g_debug ("Adding window");
        add_window (*win);
        g_debug ("Window added, now have %ld", get_windows ().size ());
        return true;
    }
    catch (Glib::Exception &e)
    {
        g_warning ("Failed to start new nvim instance: %s", e.what ().c_str());
    }
    catch (std::exception &e)
    {
        g_warning ("Failed to start new nvim instance: %s", e.what ());
    }
    return false;
}

void Application::on_window_removed (Gtk::Window *)
{
    // Apparently this is called before the window is actually removed, so
    // size still includes the window being removed
    if (get_windows ().size () <= 1)
    {
        g_debug ("No open windows, quitting");
        quit ();
    }
    else
    {
        g_debug ("Window removed, but %ld left", get_windows ().size ());
    }
}

RefPtr<Gio::ApplicationCommandLine> Application::null_cl;

}
