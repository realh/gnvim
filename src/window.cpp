/* window.cpp
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
#include "nvim-grid-view.h"
#include "window.h"

namespace Gnvim
{

Window::Window(bool maximise, int width, int height,
        std::shared_ptr<NvimBridge> nvim)
: nvim_(nvim), bufs_and_tabs_(nvim),
    maximise_(maximise), columns_(width), lines_(height),
    rqset_(RequestSet::create(sigc::mem_fun(*this, &Window::ready_to_start)))
{
    auto prom = MsgpackPromise::create();
    nvim_->get_api_info(rqset_->get_proxied_promise(prom));
    if (width == -1)
    {
        columns_ = 80;
        prom = MsgpackPromise::create();
        prom->value_signal().connect
            (sigc::mem_fun(*this, &Window::on_columns_response));
        nvim_->nvim_get_option("columns", rqset_->get_proxied_promise(prom));
    }
    if (height == -1)
    {
        lines_ = 30;
        prom = MsgpackPromise::create();
        prom->value_signal().connect
            (sigc::mem_fun(*this, &Window::on_lines_response));
        nvim_->nvim_get_option("lines", rqset_->get_proxied_promise(prom));
    }
    rqset_->ready();
}

Window::~Window()
{
    //g_debug("Window deleted");
    nvim_->stop();
    delete view_;
}

void Window::ready_to_start(RequestSet *)
{
    bat_conn_ = bufs_and_tabs_.signal_got_all_info().connect([this]()
    {
        view_ = new NvimGridView(nvim_, columns_, lines_);
        rqset_.reset();
        nvim_->start_ui(columns_, lines_);
        add(*view_);
        view_->show_all();
        if (maximise_)
        {
            maximize();
        }
        else
        {
            Gtk::Requisition minimum, natural;
            view_->get_preferred_size(minimum, natural);
            set_default_size(natural.width, natural.height);
        }

        nvim_->io_error_signal().connect(
                sigc::mem_fun(*this, &Window::on_nvim_error));

        set_geometry_hints();

        present();
        nvim_->redraw_set_title.connect
            (sigc::mem_fun(this, &Window::on_redraw_set_title));
        bat_conn_.disconnect();
    });
    bufs_and_tabs_.start();
}

void Window::force_close()
{
    force_close_ = true;
    close();
}

void Window::on_nvim_error(Glib::ustring desc)
{
    g_debug("Closing window due to nvim communication error: %s",
            desc.c_str());
    force_close();
}

void Window::on_redraw_set_title(const std::string &title)
{
    set_title(title);
}

void Window::on_columns_response(const msgpack::object &o)
{
    o.convert_if_not_nil(columns_);
}

void Window::on_lines_response(const msgpack::object &o)
{
    o.convert_if_not_nil(lines_);
}

/*
void Window::on_size_allocate(Gtk::Allocation &alloc)
{
    g_debug(">> Window::size_allocate %dx%d",
            alloc.get_width(), alloc.get_height());
    Gtk::ApplicationWindow::on_size_allocate(alloc);
    g_debug("<< Window::size_allocate %dx%d",
            alloc.get_width(), alloc.get_height());
}
*/

bool Window::on_delete_event(GdkEventAny *e)
{
    if (force_close_ || !bufs_and_tabs_.any_modified())
        return Gtk::ApplicationWindow::on_delete_event(e);

    DcsDialog dcs(*this);
    switch (dcs.show_and_run())
    {
        case DcsDialog::DISCARD:
            nvim_discard_all();
            return Gtk::ApplicationWindow::on_delete_event(e);
        case DcsDialog::SAVE:
            nvim_save_all();
            return Gtk::ApplicationWindow::on_delete_event(e);
        default:
            return true;
    }
}

#if 0
void Window::set_geometry_hints()
{
    Gdk::Geometry geom;
    view_->get_cell_size_in_pixels(geom.width_inc, geom.height_inc);
    // Use max_ as placeholder for total size
    view_->get_preferred_size(geom.max_width, geom.max_height);
    geom.max_width += get_margin_left() + get_margin_right();
    geom.max_height += get_margin_top() + get_margin_bottom();
    // Use min_ as placeholder for number of cells
    view_->get_allocation_in_cells(geom.min_width, geom.min_height);
    geom.base_width = geom.max_width - geom.min_width * geom.width_inc;
    geom.base_height = geom.max_height - geom.min_height * geom.height_inc;
    geom.min_width = 5 * geom.width_inc + geom.base_width;
    geom.min_height = 4 * geom.height_inc + geom.base_height;
    g_debug("Base size %dx%d, increments %dx%d",
            geom.base_width, geom.base_height,
            geom.width_inc, geom.height_inc);
    Gtk::ApplicationWindow::set_geometry_hints(*view_, geom,
            Gdk::HINT_MIN_SIZE | Gdk::HINT_RESIZE_INC | Gdk::HINT_BASE_SIZE);
}
#endif

RefPtr<Gio::ApplicationCommandLine> Window::null_cl;

}
