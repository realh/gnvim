/* text-grid.h
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

#ifndef GNVIM_TEXT_GRID_H
#define GNVIM_TEXT_GRID_H

#include "defns.h"

#include <set>
#include <vector>

#include <cairomm/cairomm.h>

#include "cell-attributes.h"

namespace Gnvim
{

/** A grid of text with style attributes for representing a neovim display.
 *  Line and column numbers are indexed from 0.
 */
class TextGrid {
private:
    class TextCell {
    public:
        /**
         * @param attrs this does not take ownership of attrs
         */
        TextCell (gunichar c = ' ', const CellAttributes *attrs = nullptr)
            : c_ (c), attrs_ (attrs)
        {}

        /**
         * @param attrs this does not take ownership of attrs
         */
        TextCell (const Glib::ustring &c, const CellAttributes *attrs = nullptr)
            : attrs_ (attrs)
        {
            set_text (c);
        }

        void set_char (gunichar c)
        {
            c_ = c;
        }

        void clear ()
        {
            set_char (' ');
            clear_attrs ();
        }

        /**
         * @param text should contain a single unicode char which is copied
         *              into this
         */
        void set_text (const Glib::ustring &text)
        {
            c_ = text[0];
        }

        gunichar get_char () const
        {
            return c_;
        }

        const CellAttributes *get_attrs () const
        {
            return attrs_;
        }

        /**
         * @param attrs this does not take ownership of attrs
         */
        void set_attrs (const CellAttributes &attrs)
        {
            attrs_ = &attrs;
        }

        void clear_attrs ()
        {
            attrs_ = nullptr;
        }
    private:
        gunichar c_;
        const CellAttributes *attrs_;
    };
public:
    TextGrid (int columns, int lines);

    /**
     * @param text Consisting of a single unichar which is copied.
     */
    void set_text_at (const Glib::ustring &text, int column, int line)
    {
        grid_[line * columns_ + column].set_text (text);
    }

    /// Clears the entire grid of text and styles.
    void clear ();

    /// Clears text and styles from column (inclusive) to the end of the line.
    void clear_eol (int column, int line);

    /**
     * @param attrs is not used directly, but looked up in attrs_ or copied
     *              into attrs_ first.
     * The column and line values are all inclusive.
     */
    void apply_attrs (const CellAttributes &attrs,
            int start_column, int start_line, int end_column, int end_line);

    /**
     * Draws (part of) the line of text in the given cairo context.
     * Columns are inclusive.
     */
    void draw_line (Cairo::RefPtr<Cairo::Context> cairo,
            int line, int start_column, int end_column);

    /** Scroll a region of the grid. 
     * The limits are all inclusive.
     * @param count Distance to move by in units of lines; positive to move
     *              text up, negative for down. 
     */
    void scroll (int left, int top, int right, int bottom, int count);
private:
    int columns_, lines_;
    std::vector<TextCell> grid_;
    std::set<CellAttributes> attrs_;
    const CellAttributes &default_attrs_;
};

}

#endif // GNVIM_TEXT_GRID_H
