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

#include "buffer.h"
#include "nvim-bridge.h"

namespace Gnvim
{

class Window : public Gtk::ApplicationWindow {
public:
    Window (std::vector<const char *> args);

    void force_close ();
private:
    void on_nvim_error (Glib::ustring desc);

    void on_nvim_resize (int columns, int rows);

    // Geometry hints have a history of breakage and serve almost no useful
    // purpose in current desktops
    void set_geometry_hints () {}

    bool force_close_ {false};

    NvimBridge nvim_;

    // These is just a convenience pointer, view is managed by GTK
    View *view_;

    Buffer *buffer_;
};

}

#endif // GNVIM_WINDOW_H
