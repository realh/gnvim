/* app.h
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

#ifndef GNVIM_APP_H
#define GNVIM_APP_H

#include "defns.h"

#include <vector>

namespace Gnvim
{

class Application : public Gtk::Application {
public:
    Application ();

    static RefPtr<Application> create ()
    {
        return RefPtr<Application> (new Application ());
    }

    virtual void on_activate () override;

    virtual int on_command_line (const RefPtr<Gio::ApplicationCommandLine> &)
            override;

    void open_window ()
    {
        std::vector<const char *> no_args;
        open_window (no_args);
    }

    void open_window (const std::vector<const char *> &args);
};

}

#endif // GNVIM_APP_H
