/* window.cpp
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

#include "view.h"
#include "window.h"

namespace Gnvim
{

Window::Window (std::vector<const char *>)
{
    auto view = new View (80, 36);
    int width, height;
    view->get_preferred_size (width, height);
    set_default_size (width, height);

    view->show_all ();
    add (*view);
    present ();
}

}
