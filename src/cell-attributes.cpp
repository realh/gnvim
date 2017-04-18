/* cell-attributes.cpp
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

#include <utility>

#include "cell-attributes.h"

namespace Gnvim
{

CellAttributes::CellAttributes (const CellAttributes &other)
    : foreground_rgb_ (other.foreground_rgb_),
    background_rgb_ (other.background_rgb_),
    special_rgb_ (other.special_rgb_ | DIRTY_BIT)
{}

CellAttributes &CellAttributes::operator= (const CellAttributes &other)
{
    foreground_rgb_ = other.foreground_rgb_;
    background_rgb_  = other.background_rgb_;
    special_rgb_  = other.special_rgb_ | DIRTY_BIT;
    return *this;
}

CellAttributes::CellAttributes (CellAttributes &&other)
    : foreground_rgb_ (other.foreground_rgb_),
    background_rgb_ (other.background_rgb_),
    special_rgb_ (other.special_rgb_),
    pango_attrs_ (std::move(other.pango_attrs_))
{
    other.special_rgb_ |= DIRTY_BIT;
}

CellAttributes &CellAttributes::operator= (CellAttributes &&other)
{
    foreground_rgb_ = other.foreground_rgb_;
    background_rgb_  = other.background_rgb_;
    special_rgb_  = other.special_rgb_;
    pango_attrs_ = std::move(other.pango_attrs_);
    other.special_rgb_ |= DIRTY_BIT;
    return *this;
}

void CellAttributes::reset ()
{
    foreground_rgb_ = 0;
    background_rgb_ = 0xffffff;
    special_rgb_ = 0xff0000 | DIRTY_BIT;
}

void CellAttributes::set_bold (bool bold)
{
    if (bold)
    {
        special_rgb_ |= BOLD_BIT | DIRTY_BIT;
    }
    else
    {
        special_rgb_ &= ~BOLD_BIT;
        special_rgb_ |= DIRTY_BIT;
    }
}

void CellAttributes::set_italic (bool italic)
{
    if (italic)
    {
        special_rgb_ |= ITALIC_BIT | DIRTY_BIT;
    }
    else
    {
        special_rgb_ &= ~ITALIC_BIT;
        special_rgb_ |= DIRTY_BIT;
    }
}

void CellAttributes::set_reverse (bool reverse)
{
    if (reverse)
    {
        special_rgb_ |= REVERSE_BIT | DIRTY_BIT;
    }
    else
    {
        special_rgb_ &= ~REVERSE_BIT;
        special_rgb_ |= DIRTY_BIT;
    }
}

void CellAttributes::set_underline (bool underline)
{
    if (underline)
    {
        special_rgb_ &= ~UNDERCURL_BIT;
        special_rgb_ |= UNDERLINE_BIT | DIRTY_BIT;
    }
    else
    {
        special_rgb_ &= ~UNDERLINE_BIT;
        special_rgb_ |= DIRTY_BIT;
    }
}

void CellAttributes::set_undercurl (bool undercurl)
{
    if (undercurl)
    {
        special_rgb_ &= ~UNDERLINE_BIT;
        special_rgb_ |= UNDERCURL_BIT | DIRTY_BIT;
    }
    else
    {
        special_rgb_ &= ~UNDERCURL_BIT;
        special_rgb_ |= DIRTY_BIT;
    }
}

// Hashability for std::set etc
bool CellAttributes::operator< (const CellAttributes &other) const
{
    if ((special_rgb_ & ~DIRTY_BIT) == (other.special_rgb_ & ~DIRTY_BIT))
    {
        if (foreground_rgb_ == other.foreground_rgb_)
            return background_rgb_ < other.background_rgb_;
        else
            return foreground_rgb_ < other.foreground_rgb_;
    }
    return (special_rgb_ & ~DIRTY_BIT) < (other.special_rgb_ & ~DIRTY_BIT);
}

bool CellAttributes::operator== (const CellAttributes &other) const
{
    if (this == &other)
        return true;
    return ((special_rgb_ & ~DIRTY_BIT)
            == (other.special_rgb_ & ~DIRTY_BIT))
        && (background_rgb_ == other.background_rgb_)
        && (foreground_rgb_ == other.foreground_rgb_);
}

const Pango::AttrList &CellAttributes::get_pango_attrs () const
{
    if (is_dirty ())
    {
        pango_attrs_ = Pango::AttrList ();
        auto c = create_colour_attr 
                (Pango::Attribute::create_attr_foreground,
                 get_reverse () ? background_rgb_ : foreground_rgb_);
        pango_attrs_.insert (c);
        c = create_colour_attr 
                (Pango::Attribute::create_attr_background,
                 get_reverse () ? foreground_rgb_ : background_rgb_);
        pango_attrs_.insert (c);
        if (has_underline_or_curl ())
        {
            // pangomm is missing Pango::Attribute::create_attr_underline_color
            // https://bugzilla.gnome.org/show_bug.cgi?id=781059
            guint16 r, g, b;
            decompose_colour (special_rgb_ & 0xffffff, r, g, b);
            auto sc = Pango::Attribute
                (pango_attr_underline_color_new (r, g, b));
            pango_attrs_.insert (sc);
            auto uls = Pango::Attribute::create_attr_underline
                (get_underline ()
                 ? Pango::UNDERLINE_SINGLE : Pango::UNDERLINE_ERROR);
            pango_attrs_.insert (uls);
        }
        if (get_bold ())
        {
            auto b = Pango::Attribute::create_attr_weight (Pango::WEIGHT_BOLD);
            pango_attrs_.insert (b);
        }
        if (get_italic ())
        {
            auto i = Pango::Attribute::create_attr_style (Pango::STYLE_ITALIC);
            pango_attrs_.insert (i);
        }
    }
    return pango_attrs_;
}

Pango::AttrColor CellAttributes::create_colour_attr (
        Pango::AttrColor (*func)(guint16 r, guint16 g, guint16 b), guint32 rgb)
{
    guint16 r, g, b;
    decompose_colour (rgb, r, g, b);
    return func (r, g, b);
}

void CellAttributes::decompose_colour (guint32 rgb,
        guint16 &red, guint16 &green, guint16 &blue)
{
    auto r = rgb & 0xff0000;
    auto g = rgb & 0xff00;
    auto b = rgb & 0xff;
    red = (r >> 8) | (r >> 16);
    green = g | (g >> 8);
    blue = (b << 8) | b;
}

}
