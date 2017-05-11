/* dcs-dialog.cpp
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

#include "defns.h"

#include "dcs-dialog.h"

namespace Gnvim
{

DcsDialog::DcsDialog(Gtk::Window &parent) : Gtk::MessageDialog(
    parent, _("One or more buffers contain(s) unsaved data"),
    false, Gtk::MESSAGE_WARNING,
    Gtk::BUTTONS_NONE, true)
{
    add_button(_("Discard"), DISCARD);
    add_button(_("Save"), SAVE);
    add_button(_("Keep Open"), KEEP_OPEN);
}

int DcsDialog::show_and_run()
{
    show_all();
    present();
    return run();
}

}
