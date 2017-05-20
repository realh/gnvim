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

#include "app.h"
#include "dcs-dialog.h"
#include "nvim-grid-view.h"
#include "tab-page.h"
#include "window.h"

namespace Gnvim
{

Window::Window(bool maximise, int width, int height,
        std::shared_ptr<NvimBridge> nvim)
: nvim_(nvim), bufs_and_tabs_(nvim),
    maximise_(maximise), columns_(width), lines_(height),
    tabs_(new std::vector<TabPage>())
{
    auto rqset = RequestSet::create([this](RequestSet *rq)
    { on_options_read(rq); });
    auto prom = MsgpackPromise::create();
    nvim_->get_api_info(rqset->get_proxied_promise(prom));
    if (width == -1)
    {
        columns_ = 80;
        prom = MsgpackPromise::create();
        prom->value_signal().connect([this](const msgpack::object &o)
        {
            o.convert_if_not_nil(columns_);
        });
        nvim_->nvim_get_option("columns", rqset->get_proxied_promise(prom));
    }
    if (height == -1)
    {
        lines_ = 30;
        prom = MsgpackPromise::create();
        prom->value_signal().connect([this](const msgpack::object &o)
        {
            o.convert_if_not_nil(lines_);
        });
        nvim_->nvim_get_option("lines", rqset->get_proxied_promise(prom));
    }

    show_tab_line_ = 1;
    prom = MsgpackPromise::create();
    prom->value_signal().connect([this](const msgpack::object &o)
    {
        o.convert_if_not_nil(show_tab_line_);
    });
    nvim_->nvim_get_option("showtabline", rqset->get_proxied_promise(prom));

    rqset->ready();
}

Window::~Window()
{
    //g_debug("Window deleted");
    nvim_->stop();
    // Need to make sure these are deleted in the right order
    delete grid_;
    delete notebook_;
    delete tabs_;
    delete view_;
}

void Window::on_options_read(RequestSet *)
{
    bat_conn_ = bufs_and_tabs_.signal_got_all_info().connect
        (sigc::mem_fun(*this, &Window::ready_to_start));
    bufs_and_tabs_.start();
}

void Window::ready_to_start()
{
    // For some reason these container widgets don't have create methods
    grid_ = new Gtk::Grid();
    add(*grid_);

    notebook_ = new Gtk::Notebook();
    show_or_hide_tabs();
    grid_->attach(*notebook_, 0, 0, 1, 1);

    for (const auto &tab: bufs_and_tabs_.get_tabs())
    {
        tabs_->emplace_back(tab);
        auto &page = tabs_->back();
        auto &label = page.get_label_widget();
        page.show_all();
        label.show_all();
        notebook_->append_page(page, label);
        g_debug("Adding notebook page");
    }

    view_ = new NvimGridView(nvim_, columns_, lines_);

    nvim_->start_ui(columns_, lines_);
    grid_->attach(*view_, 0, 1, 1, 1);

    // Show everything from the Box downwards before getting requisition
    view_->show_all();
    grid_->show();

    view_->set_can_focus(true);
    view_->grab_focus();
    view_->set_focus_on_click(true);

    if (maximise_)
    {
        maximize();
    }
    else
    {
        Gtk::Requisition minimum, natural;
        grid_->get_preferred_size(minimum, natural);
        set_default_size(natural.width, natural.height);
    }

    nvim_->io_error_signal().connect(
            sigc::mem_fun(*this, &Window::on_nvim_error));

    set_geometry_hints();

    present();
    nvim_->redraw_set_title.connect
        (sigc::mem_fun(this, &Window::on_redraw_set_title));
    bat_conn_.disconnect();
}

void Window::show_or_hide_tabs()
{
    show_or_hide_tabs(Application::app_gsettings()->get_boolean("gui-tabs"));
}

void Window::show_or_hide_tabs(bool show)
{
    if (show)
        notebook_->show_all();
    else
        notebook_->hide();
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
