/* text-grid.cpp
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

#include <cstring>

#include "text-grid.h"

namespace Gnvim
{

TextGrid::TextGrid(int columns, int lines, int cell_width, int cell_height)
    : columns_(columns), lines_(lines),
    cell_width_(cell_width), cell_height_(cell_height),
    grid_(columns_ * lines_)
{
}

void TextGrid::resize(int columns, int lines)
{
    columns_ = columns;
    lines_ = lines;
    grid_.resize(columns * lines);
}

void TextGrid::clear()
{
    for (auto &cell: grid_)
        cell.clear();
}

void TextGrid::clear(int left, int top, int right, int bottom)
{
    top *= columns_;
    bottom *= columns_;
    for (; top <= bottom; ++top)
    {
        for (int col = left; col <= right; ++col)
        {
            grid_[top + col].clear();
        }
    }
}

void TextGrid::apply_attrs(const CellAttributes &attrs,
        int start_column, int start_line, int end_column, int end_line)
{
    auto start = start_line * columns_ + start_column;
    auto end = end_line * columns_ + end_column;
    const auto &it = attrs_.find(attrs);
    const auto &actual_attrs = (it == attrs_.end())
        ? *((attrs_.emplace(attrs)).first) : *it;
    for (int i = start; i <= end; ++i)
    {
        auto &cell = grid_[i];
        cell.set_attrs(actual_attrs);
    }
}

void TextGrid::scroll(int left, int top, int right, int bottom, int count)
{
    if(!count)
        return;
    int start, end, step;
    auto dat = grid_.data();
    if(count > 0)  // up
    {
        start = top;
        end = bottom + 1 - count;
        step = 1;
    }
    else    // down
    {
        start = bottom;
        end = top - 1 - count;
        step = -1;
    }
    for (auto y = start; y != end; y += step)
    {
        std::memmove(dat + y * columns_, dat + (y + count) * columns_,
                (right - left + 1) * sizeof(TextCell));
    }
    if(count > 0)  // up
    {
        start = (bottom - count + 1) * columns_;
        end = (bottom + 1) * columns_;
    }
    else    // down
    {
        start = top * columns_;
        end = (top - count) * columns_;
    }
    //g_debug("Scroll clearing text in lines %d-%d(count %d)",
    //        start / columns_, end / columns_ - 1, count);
    for (auto y = start; y != end; y += columns_)
    {
        for (auto x = left; x <= right; ++x)
        {
            auto &cell = grid_[y + x];
            cell.clear();
        }
    }
}

void TextGrid::draw_line(const Cairo::RefPtr<Cairo::Context> &cairo,
        int line, int start_column, int end_column,
        const CellAttributes *override_attrs)
{
    auto layout = Pango::Layout::create(cairo);
    auto li = line * columns_;
    Glib::ustring s;
    auto last_attrs = override_attrs ? override_attrs
        : grid_[li + start_column].get_attrs();
    int last_x = start_column;
    int y = line * cell_height_;

    layout->set_font_description(*font_);

    for (int x = start_column; x <= end_column + 1; ++x)
    {
        const auto cell = (x <= end_column) ? &grid_[li + x] : nullptr;
        auto current_attrs = override_attrs ? override_attrs
            : (cell ? cell->get_attrs() : nullptr);
        if(x > end_column || current_attrs != last_attrs)
        {
            if(!last_attrs)
                last_attrs = &default_attrs_;

            // Need to fill background with attr's background colour
            /*
            guint16 r, g, b;
            CellAttributes::decompose_colour(last_attrs->get_background_rgb(),
                    r, g, b);
            cairo->rectangle(last_x * cell_width_, y,
                    (x - last_x) * cell_width_, cell_height_);
            cairo->fill();
            */
            /*
            g_debug("draw_line filling %d,%d+%dx%d with %06x", last_x, line,
                    x - last_x, 1, last_attrs->get_background_rgb());
            g_debug("draw_line writing '%s' at %d, %d", s.c_str(),
                    last_x, line);
            */

            layout->set_attributes(last_attrs->get_pango_attrs());
            layout->set_text(s);
            cairo->move_to(last_x * cell_width_, y);
            // update shouldn't be necessary, but python-gui does it
            layout->update_from_cairo_context(cairo);
            layout->show_in_cairo_context(cairo);
            s.clear();
            last_x = x;
            last_attrs = current_attrs;
        }
        if(x <= end_column)
        {
            s += cell->get_char();
        }
    }
}

}
