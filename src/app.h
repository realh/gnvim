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

namespace Gnvim
{

class Application : public Gtk::Application {
protected:
    Application ();
public:
    static RefPtr<Application> create ()
    {
        return RefPtr<Application> (new Application ());
    }

    virtual void on_activate () override;

    virtual int on_command_line (const RefPtr<Gio::ApplicationCommandLine> &)
            override;

    bool open_window (const RefPtr<Gio::ApplicationCommandLine> & = null_cl);

    //virtual void remove_window 
protected:
    virtual void on_window_removed (Gtk::Window *) override;
private:
    static RefPtr<Gio::ApplicationCommandLine> null_cl;
};

}

#endif // GNVIM_APP_H
