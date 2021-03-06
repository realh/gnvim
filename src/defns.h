/* defns.h
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

#ifndef GNVIM_DEFNS_H
#define GNVIM_DEFNS_H

// Build options etc may go here, so include this before anything else

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory>

#include <gtkmm.h>

#define _(s) (s)

namespace Gnvim
{

template<class T> using RefPtr = Glib::RefPtr<T>;

class Application;
class NvimBridge;
class NvimGridView;
class Window;

using PromiseHandle = std::shared_ptr<class MsgpackPromise>;

}

#endif // GNVIM_DEFNS_H
