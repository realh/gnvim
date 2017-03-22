/* buffer.h
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

#ifndef GNVIM_BUFFER_H
#define GNVIM_BUFFER_H

#include "defns.h"

namespace Gnvim
{

class Buffer : public Gtk::TextBuffer {
public:
    static Buffer *create (int columns, int rows)
    {
        return new Buffer (columns, rows);
    }

    void get_size (int &columns, int &rows)
    {
        columns = columns_;
        rows = rows_;
    }

    void resize (int columns, int rows);
protected:
    Buffer (int columns, int rows);
private:
    void init_content ();

    int columns_, rows_;
};

}

#endif // GNVIM_BUFFER_H
