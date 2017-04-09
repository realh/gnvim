/* cell-attributes.h
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

#ifndef GNVIM_CELL_ATTRIBUTES_H
#define GNVIM_CELL_ATTRIBUTES_H

#include "defns.h"

#include <pangomm.h>

namespace Gnvim
{

class CellAttributes {
private:
    constexpr static int BOLD_BIT = 0x1000000;
    constexpr static int ITALIC_BIT = 0x2000000;
    constexpr static int REVERSE_BIT = 0x4000000;
    constexpr static int UNDERLINE_BIT = 0x8000000;
    constexpr static int UNDERCURL_BIT = 0x10000000;
    constexpr static int DIRTY_BIT = 0x20000000;
public:
    CellAttributes () : foreground_rgb_ {0}, background_rgb_ {0xffffff},
                   special_rgb_ {0xff0000 | DIRTY_BIT}
    {}

    CellAttributes (const CellAttributes &);

    CellAttributes &operator=(const CellAttributes &);

    CellAttributes (CellAttributes &&);

    CellAttributes &operator=(CellAttributes &&);

    void set_foreground_rgb (guint32 rgb)
    {
        foreground_rgb_ = rgb;
        special_rgb_ |= DIRTY_BIT;
    }

    guint32 get_foreground_rgb () const
    {
        return foreground_rgb_;
    }

    void set_background_rgb (guint32 rgb)
    {
        background_rgb_ = rgb;
        special_rgb_ |= DIRTY_BIT;
    }

    guint32 get_background_rgb () const
    {
        return background_rgb_;
    }

    void set_special_rgb (guint32 rgb)
    {
        special_rgb_ &= 0xff000000;
        special_rgb_ |= (rgb & 0xffffff) | DIRTY_BIT;
    }

    guint32 get_special_rgb () const
    {
        return special_rgb_ & 0xffffff;
    }

    void set_bold (bool bold = true);

    bool get_bold () const
    {
        return special_rgb_ & BOLD_BIT;
    }

    void set_italic (bool italic = true);

    bool get_italic () const
    {
        return special_rgb_ & ITALIC_BIT;
    }

    void set_reverse (bool reverse = true);

    bool get_reverse () const
    {
        return special_rgb_ & REVERSE_BIT;
    }

    void set_underline (bool underline = true);

    bool get_underline () const
    {
        return special_rgb_ & UNDERLINE_BIT;
    }

    void set_undercurl (bool undercurl = true);

    bool get_undercurl () const
    {
        return special_rgb_ & UNDERCURL_BIT;
    }

    // Hashability for std::set etc
    bool operator< (const CellAttributes &other) const;

    bool operator== (const CellAttributes &other) const;

    const Pango::AttrList &get_pango_attrs () const;
private:
    bool is_dirty () const
    {
        return special_rgb_ & DIRTY_BIT;
    }

    bool has_underline_or_curl () const
    {
        return special_rgb_ & (UNDERLINE_BIT | UNDERCURL_BIT);
    }

    static Pango::AttrColor create_colour_attr (
        Pango::AttrColor (*func)(guint16 r, guint16 g, guint16 b), guint32 rgb);

    static void decompose_colour (guint32 rgb,
            guint16 &red, guint16 &green, guint16 &blue);

    guint32 foreground_rgb_;
    guint32 background_rgb_;
    guint32 special_rgb_;       // Other flags are stored in top 8 bits
    mutable Pango::AttrList pango_attrs_;
};

}

#endif // GNVIM_CELL_ATTRIBUTES_H
