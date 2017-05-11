/* dcs-dialog.h
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

#ifndef GNVIM_DCS_DIALOG_H
#define GNVIM_DCS_DIALOG_H

#include "defns.h"

namespace Gnvim
{

/// Dialog asking user to confirm quit/close when there is unsaved data.
/// DCS stands for Discard/Cancel/Save and was a common terminology in RISC OS.
class DcsDialog : public Gtk::MessageDialog {
public:
    enum Result {
        DISCARD = 1,
        SAVE,
        KEEP_OPEN
    };

    DcsDialog(Gtk::Window &parent);

    /// Returns a GTK stock result code or a value from the Result enum.
    int show_and_run();
};

}

#endif // GNVIM_DCS_DIALOG_H
