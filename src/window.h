/* window.h
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

#ifndef GNVIM_WINDOW_H
#define GNVIM_WINDOW_H

#include "defns.h"

#include <vector>

namespace Gnvim
{

class Window : public Gtk::ApplicationWindow {
public:
    Window (std::vector<const char *> args);
private:
    void set_geometry_hints ();

    // This is just a convenience pointer, view is managed by super class
    View *view_;
};

}

#endif // GNVIM_WINDOW_H
