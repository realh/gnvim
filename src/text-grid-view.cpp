/* text-grid-view.cpp
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

#include "text-grid-view.h"

namespace Gnvim
{

TextGridView::TextGridView (int columns, int lines,
        const std::string &font_name)
    : grid_ (columns, lines),
    columns_ (columns), lines_ (lines),
    font_ (font_name.length () ? font_name : DEFAULT_FONT)
{
    calculate_metrics ();
}

void TextGridView::set_font (const Glib::ustring &desc, bool q_resize)
{
    font_ = Pango::FontDescription (desc.length () ? desc : DEFAULT_FONT);
    calculate_metrics ();
    if (q_resize)
        resize_window ();
}

void TextGridView::resize_window ()
{
    auto view_alloc = get_allocation ();
    int _, nat_width, nat_height;
    get_preferred_width_vfunc (_, nat_width);
    get_preferred_height_vfunc (_, nat_height);
    bool changed = nat_width != view_alloc.get_width ()
                || nat_height != view_alloc.get_height ();
    auto win = static_cast<Gtk::Window *> (get_toplevel ());
    if (changed)
    {
        // I wasn't sure whether the queue_resize and set_size_request were
        // necessary. Things seem to work correctly without them, whereas with
        // them, the window initially resizes correctly, but in Wayland (and
        // in X11 with CSD?) if the size change is a reduction in width it
        // gains some of it back the next time the window is focused. Weird.

        //queue_resize ();
        if (win)
        {
            int w = nat_width + toplevel_width_ - view_alloc.get_width ();
            int h = nat_height + toplevel_height_ - view_alloc.get_height ();
            //win->set_size_request (w, h);
            win->resize (w, h);
        }
    }
}

void TextGridView::on_parent_changed (Gtk::Widget *old_parent)
{
    Gtk::DrawingArea::on_parent_changed (old_parent);
    // The width and height sent in Window::size-allocate signals are no use
    // because of CSD and Wayland issues, so we're supposed to use
    // gtk_window_get_size. But that should only be used during certain signal
    // handlers, size-allocate being one of them.
    if (old_parent && toplevel_size_allocate_connection_)
        toplevel_size_allocate_connection_.disconnect ();
    toplevel_size_allocate_connection_
        = get_toplevel ()->signal_size_allocate ().connect
            (sigc::mem_fun (this, &TextGridView::on_toplevel_size_allocate));
}

void TextGridView::on_size_allocate (Gtk::Allocation &allocation)
{
    Gtk::DrawingArea::on_size_allocate (allocation);
    int w = allocation.get_width ();
    int h = allocation.get_height ();
    // Size requests and allocations appear not to include margins
    columns_ = w / cell_width_px_;
    lines_ = h / cell_height_px_;

    if (grid_.get_columns () != columns_ || grid_.get_lines () != lines_)
    {
        grid_.resize (columns_, lines_);
        grid_cr_.clear ();
        grid_surface_.clear ();
    }
    g_debug ("on_size_allocate");
    create_cairo_surface ();
}

void TextGridView::on_realize ()
{
    Gtk::DrawingArea::on_realize ();
    g_debug ("on_realize");
    create_cairo_surface ();
}

void TextGridView::create_cairo_surface ()
{
    if (!grid_cr_)
    {
        auto gwin = get_window ();
        if (gwin)
        {
            auto allocation = get_allocation ();
            int w = allocation.get_width ();
            int h = allocation.get_height ();
            g_debug ("Creating surface %d x %d", w, h);
            grid_surface_ = gwin->create_similar_surface
                (Cairo::CONTENT_COLOR, w, h);
            grid_cr_ = Cairo::Context::create (grid_surface_);
            redraw_view ();
        }
        else
        {
            g_debug ("No GdkWindow");
        }
    }
    else
    {
        g_debug ("Already have a cairo surface");
    }
}

void TextGridView::redraw_view ()
{
    clear_view ();
    for (int line = 0; line < lines_; ++line)
        grid_.draw_line (grid_cr_, line, 0, columns_ - 1);
}

void TextGridView::scroll (int left, int top, int right, int bottom, int count)
{
    grid_.scroll (left, top, right, bottom, count);

    int src_top, dest_top, clear_top, copy_height;
    int left_px = left * cell_width_px_;
    int width_px = (right - left + 1) * cell_width_px_;

    if (count > 0)
    {
        dest_top = top;
        src_top = top + count;
        copy_height = bottom - top + 1 - count;
        clear_top = bottom + 1 - count;
    }
    else
    {
        src_top = top;
        dest_top = top - count;
        copy_height = bottom - top + 1 + count;
        clear_top = top;
    }

    src_top *= cell_height_px_;
    dest_top *= cell_height_px_;
    copy_height *= cell_height_px_;

    // Create a surface which is a copy of the region to be moved.
    auto copy_surf = Cairo::Surface::create (grid_surface_,
            Cairo::CONTENT_COLOR, width_px, copy_height);
    auto copy_cr = Cairo::Context::create (copy_surf);
    copy_cr->set_source (grid_surface_, -left_px, -src_top);
    copy_cr->paint ();

    // Paint the movable surface to its new position
    grid_cr_->save ();
    grid_cr_->rectangle (left_px, dest_top, width_px, copy_height);
    grid_cr_->clip ();
    grid_cr_->set_source (copy_surf, left_px, dest_top);
    grid_cr_->paint ();
    grid_cr_->restore ();

    // Clear the area "uncovered" by the moved region
    fill_background (grid_cr_, left, clear_top, right, clear_top + count - 1);
}

void TextGridView::fill_background (Cairo::RefPtr<Cairo::Context> cr,
        int left, int top, int right, int bottom)
{
    guint16 r, g, b;
    CellAttributes::decompose_colour (grid_.get_default_background_rgb (),
            r, g, b);
    cr->set_source_rgb (r, g, b);
    cr->rectangle (left * cell_width_px_, top * cell_height_px_,
            (right - left + 1) * cell_width_px_,
            (bottom - top + 1) * cell_height_px_);
    cr->fill ();
}

void TextGridView::calculate_metrics ()
{
    auto pc = get_pango_context ();
    auto metrics = pc->get_metrics (font_);
    // digit_width and char_width should be the same for monospace, but
    // digit_width might be slightly more useful in case we accidentally use
    // a proportional font
    cell_width_px_ = metrics.get_approximate_digit_width () / PANGO_SCALE;
    cell_height_px_ = (metrics.get_ascent () + metrics.get_descent ())
            / PANGO_SCALE;
    grid_.set_cell_size (cell_width_px_, cell_height_px_);
    grid_.set_font (font_);
}

void TextGridView::get_preferred_width_vfunc (int &minimum, int &natural) const
{
    auto cols = grid_.get_columns ();
    minimum = 5 * cell_width_px_;
    natural = cols * cell_width_px_;
    //g_debug ("Preferred width bw %d + %d columns * %dpx = %d",
    //        bw, cols, cell_width_px_, natural);
}

void TextGridView::get_preferred_height_vfunc (int &minimum, int &natural) const
{
    auto lines = grid_.get_lines ();
    minimum = 5 * cell_height_px_;
    natural = lines * cell_height_px_;
    //g_debug ("Preferred height bh %d + %d lines * %dpx = %d",
    //        bh, lines, cell_height_px_, natural);
}

void TextGridView::on_toplevel_size_allocate (Gtk::Allocation &)
{
    static_cast<Gtk::Window *> (get_toplevel ())->get_size
        (toplevel_width_, toplevel_height_);
}

bool TextGridView::on_draw (const Cairo::RefPtr<Cairo::Context> &cr)
{
    auto alloc = get_allocation ();
    int w = alloc.get_width ();
    int h = alloc.get_height ();
    grid_surface_->flush ();
    cr->save ();
    cr->rectangle (0, 0, w, h);
    cr->clip ();
    double l, t, r, b;
    cr->get_clip_extents (l, t, r, b);
    g_debug ("redraw clip extents L:%f T:%f R:%f B:%f", l, t, r, b);
    cr->set_source (grid_surface_, 0, 0);
    cr->paint ();
    cr->restore ();
    return true;
}

}
