/* text-grid-view.cpp
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

#include "app.h"
#include "text-grid-view.h"
#include "text-grid-widget.h"

namespace Gnvim
{

TextGridView::TextGridView(int columns, int lines,
        const std::string &font_name)
    : grid_(columns, lines),
    columns_(columns), lines_(lines),
    font_(font_name.length() ? font_name : DEFAULT_FONT)
{
    auto settings = Application::sys_gsettings();
    cursor_blinks_ = settings->get_boolean("cursor-blink");
    cursor_blink_cx_ = settings->signal_changed("cursor-blink").connect
        (sigc::mem_fun(*this, &TextGridView::on_cursor_gsetting_changed));
    cursor_interval_ = settings->get_int("cursor-blink-time") / 2;
    cursor_blink_time_cx_ = settings->signal_changed
        ("cursor-blink-time").connect
            (sigc::mem_fun(*this, &TextGridView::on_cursor_gsetting_changed));
    cursor_timeout_ = settings->get_int("cursor-blink-timeout") * 1000000;
    cursor_blink_timeout_cx_ = settings->signal_changed
        ("cursor-blink-timeout").connect
            (sigc::mem_fun(*this, &TextGridView::on_cursor_gsetting_changed));
    cursor_attr_.set_background_rgb(grid_.get_default_foreground_rgb());
    cursor_attr_.set_foreground_rgb(grid_.get_default_background_rgb());
    show_cursor();
}

TextGridView::~TextGridView()
{
    cursor_blink_cx_.disconnect();
    cursor_blink_time_cx_.disconnect();
    cursor_blink_timeout_cx_.disconnect();
    if (cursor_cx_.connected())
        cursor_cx_.disconnect();
}

void TextGridView::set_current_widget(Gtk::Widget *w)
{
    current_widget_ = static_cast<TextGridWidget *>(w);
}

void TextGridView::show_cursor()
{
    cursor_idle_at_ = g_get_monotonic_time() + cursor_timeout_;
    cursor_visible_ = false;
    on_cursor_blink();
    if (cursor_cx_.connected())
        cursor_cx_.disconnect();
    if (cursor_blinks_ && current_widget_ && current_widget_->has_focus())
    {
        cursor_cx_ = Glib::signal_timeout().connect
            (sigc::mem_fun(*this, &TextGridView::on_cursor_blink),
             cursor_interval_);
    }
}

bool TextGridView::on_cursor_blink()
{
    bool no_blink = (g_get_monotonic_time() - cursor_idle_at_ > 0)
        || !cursor_blinks_ || !current_widget_ || !current_widget_->has_focus();
    cursor_visible_ = no_blink ? true : !cursor_visible_;
    if (current_widget_)
    {
        current_widget_->queue_draw_area(cursor_col_ * cell_width_px_,
                cursor_line_ * cell_height_px_,
                cell_width_px_, cell_height_px_);
    }
    return !no_blink;
}

void TextGridView::on_cursor_gsetting_changed(const Glib::ustring &key)
{
    auto settings = Application::sys_gsettings();
    if (key == "cursor-blink")
        cursor_blinks_ = settings->get_int("cursor-blink");
    else if (key == "cursor-blink-time")
        cursor_interval_ = settings->get_int("cursor-blink-time") / 2;
    else if (key == "cursor-blink-timeout")
    {
        cursor_timeout_ = settings->get_int("cursor-blink-timeout") * 1000000;
        return;
    }
    show_cursor();
}

void TextGridView::set_font(RefPtr<Pango::Context> pc,
            const Glib::ustring &desc, bool resize)
{
    font_ = Pango::FontDescription(desc.length() ? desc : DEFAULT_FONT);
    calculate_metrics(pc);
    if (resize)
    {
        resize_surface();
        redraw_view();
    }
}

void TextGridView::resize_surface()
{
    grid_surface_ = Cairo::Surface::create(grid_surface_, Cairo::CONTENT_COLOR,
         cell_width_px_ * columns_,
         cell_height_px_ * lines_);
    grid_cr_ = Cairo::Context::create(grid_surface_);
}

void TextGridView::on_size_allocate(Gtk::Allocation &allocation)
{
    int w = allocation.get_width();
    int h = allocation.get_height();
    //g_debug("TextGridView size_allocate %d x %d", w, h);
    // Size requests and allocations appear not to include margins
    columns_ = w / cell_width_px_;
    lines_ = h / cell_height_px_;

    if (grid_.get_columns() != columns_ || grid_.get_lines() != lines_)
    {
        grid_.resize(columns_, lines_);
        if (grid_surface_)
            resize_surface();
    }
}

void TextGridView::create_cairo_surface(Gtk::Widget *w)
{
    if (!grid_cr_)
    {
        auto gwin = w->get_window();
        if (gwin)
        {
            calculate_metrics(w->get_pango_context());
            grid_surface_ = gwin->create_similar_surface
                (Cairo::CONTENT_COLOR,
                 cell_width_px_ * columns_,
                 cell_height_px_ * lines_);
            grid_cr_ = Cairo::Context::create(grid_surface_);
            redraw_view();
        }
        else
        {
            g_warning("No GdkWindow to create cairo surface");
        }
    }
    /*
    else
    {
        g_debug("Already have a cairo surface");
    }
    */
}

void TextGridView::redraw_view()
{
    clear_view();
    for (int line = 0; line < lines_; ++line)
        grid_.draw_line(grid_cr_, line, 0, columns_ - 1);
}

void TextGridView::scroll(int left, int top, int right, int bottom, int count)
{
    grid_.scroll(left, top, right, bottom, count);

    if (global_redraw_pending_)
        return;

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
    auto copy_surf = Cairo::Surface::create(grid_surface_,
            Cairo::CONTENT_COLOR, width_px, copy_height);
    auto copy_cr = Cairo::Context::create(copy_surf);
    copy_cr->set_source(grid_surface_, -left_px, -src_top);
    copy_cr->paint();

    // Paint the movable surface to its new position
    grid_cr_->save();
    grid_cr_->rectangle(left_px, dest_top, width_px, copy_height);
    grid_cr_->clip();
    grid_cr_->set_source(copy_surf, left_px, dest_top);
    grid_cr_->paint();
    grid_cr_->restore();

    // Clear the area "uncovered" by the moved region
    if (count < 0)
        count = -count;
    //g_debug("Scroll filling background");
    fill_background(grid_cr_, left, clear_top, right, clear_top + count - 1);
}

void TextGridView::fill_background(Cairo::RefPtr<Cairo::Context> cr,
        int left, int top, int right, int bottom, guint32 rgb)
{
    fill_background_px(cr,
            left * cell_width_px_, top * cell_height_px_,
            (right - left + 1) * cell_width_px_,
            (bottom - top + 1) * cell_height_px_, rgb);
}

void TextGridView::fill_background_px(Cairo::RefPtr<Cairo::Context> cr,
        int left, int top, int width, int height, guint32 rgb)
{
    float r, g, b;
    CellAttributes::decompose_colour_float(rgb, r, g, b);
    cr->set_source_rgb(r, g, b);
    cr->rectangle(left, top, width, height);
    cr->fill();
}

void TextGridView::calculate_metrics(RefPtr<Pango::Context> pc)
{
    auto metrics = pc->get_metrics(font_);
    // digit_width and char_width should be the same for monospace, but
    // digit_width might be slightly more useful in case we accidentally use
    // a proportional font
    cell_width_px_ = metrics.get_approximate_digit_width() / PANGO_SCALE;
    cell_height_px_ = (metrics.get_ascent() + metrics.get_descent())
            / PANGO_SCALE;
    //g_debug("calculate_metrics: %d x %d", cell_width_px_, cell_height_px_);
    grid_.set_cell_size(cell_width_px_, cell_height_px_);
    grid_.set_font(font_);
}

void TextGridView::get_preferred_width(int &minimum, int &natural) const
{
    auto cols = grid_.get_columns();
    minimum = 5 * cell_width_px_;
    natural = cols * cell_width_px_;
    //g_debug("Preferred width %d columns * %dpx = %d",
    //        cols, cell_width_px_, natural);
}

void TextGridView::get_preferred_height(int &minimum, int &natural) const
{
    auto lines = grid_.get_lines();
    minimum = 5 * cell_height_px_;
    natural = lines * cell_height_px_;
    //g_debug("Preferred height %d lines * %dpx = %d",
    //        lines, cell_height_px_, natural);
}

void TextGridView::draw_cursor(Cairo::RefPtr<Cairo::Context> cr)
{
    if (cursor_visible_)
    {
        guint32 curs_colour = cursor_attr_.get_background_rgb();

        if (current_widget_->has_focus())
        {
            if (cursor_width_)
            {
                fill_background_px(cr,
                        cursor_col_ * cell_width_px_,
                        cursor_line_ * cell_height_px_,
                        cursor_width_, cell_height_px_,
                        curs_colour);
            }
            else
            {
                fill_background(cr,
                        cursor_col_, cursor_line_, cursor_col_, cursor_line_,
                        curs_colour);
                grid_.draw_line(cr, cursor_line_, cursor_col_, cursor_col_,
                        &cursor_attr_);
            }
        }
        else
        {
            float r, g, b;
            CellAttributes::decompose_colour_float(curs_colour, r, g, b);
            cr->begin_new_path();
            cr->set_source_rgb(r, g, b);
            cr->rectangle(cursor_col_ * cell_width_px_,
                    cursor_line_ * cell_height_px_,
                    cell_width_px_, cell_height_px_);
            cr->set_line_width(1);
            cr->stroke();
        }
    }

}

}
