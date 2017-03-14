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
    g_print ("Created Application\n");
}

void Application::on_activate ()
{
    g_print ("on_activate\n");
    open_window ();
}

int Application::on_command_line (const RefPtr<Gio::ApplicationCommandLine> &cl)
{
    g_print ("on_command_line\n");
    int argc;
    char **argv = cl->get_arguments (argc);
    std::vector<const char *> args_vec (argv, argv + argc);
    open_window (args_vec);
    g_strfreev(argv);
    return 0;
}

void Application::open_window (const std::vector<const char *> &args)
{
    auto win  = new Window (args);
    add_window (*win);
}

}
