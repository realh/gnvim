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
    int argc;
    char **argv = cl->get_arguments (argc);
    std::vector<const char *> args_vec (argv, argv + argc);
    g_strfreev(argv);
    if (!open_window (args_vec))
    {
        g_critical ("Failed to open window, exiting this instance");
        return 1;
    }
    return 0;
}

bool Application::open_window (const std::vector<const char *> &args)
{
    try {
        auto win = new Window (args);
        if (win->is_visible ())
        {
            g_debug ("Adding window");
            add_window (*win);
            return true;
        }
        else
        {
            g_debug ("Window was never opened");
        }
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
    if (!get_windows ().size ())
    {
        g_debug ("No open windows, quitting");
        // quit ();
    }
}

}
