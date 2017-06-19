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

#include <iterator>

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
    tabs_(new std::vector<std::unique_ptr<TabPage>>())
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
    delete notebook_;
    delete tabs_;
    delete view_w_;
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
    view_ = new NvimGridView(nvim_, columns_, lines_, get_pango_context());

    // For some reason (certain) container widgets don't have create methods
    notebook_ = new Gtk::Notebook();
    show_or_hide_tabs();

    int pnum = 0;
    int current = 0;
    for (const auto &tab: bufs_and_tabs_.get_tabs())
    {
        create_tab_page(tab);
        if (tab.handle == bufs_and_tabs_.get_current_tab()->handle)
        {
            current = pnum;
        }
        ++pnum;
    }
    notebook_->set_current_page(current);
    notebook_->signal_switch_page().connect([this](Gtk::Widget *w, int)
    {
        const auto &handle = static_cast<TabPage *>(w)->get_vim_handle();
        if (handle != bufs_and_tabs_.get_current_tab()->handle)
        {
            nvim_->nvim_set_current_tabpage(handle);
        }
        //view_w_->grab_focus();
    });
    nvim_->signal_bufenter.connect([this](const VimTabpage &handle)
    {
        if ((*tabs_)[notebook_->get_current_page()]->get_vim_handle() == handle)
            return;
        auto children = notebook_->get_children();
        std::size_t n;
        for (n = 0; n < children.size(); ++n)
        {
            auto page = static_cast<TabPage *>(children[n]);
            if (page->get_vim_handle() == handle)
            {
                notebook_->set_current_page(n);
                break;
            }
        }
        if (n == children.size())
        {
            g_critical("Can't find nvim's current tab in notebook");
        }
    });

    nvim_->start_ui(columns_, lines_);
    add(*notebook_);

    // Show everything before getting requisition
    notebook_->show();

    /*
    view_->set_can_focus(true);
    view_->grab_focus();
    view_->set_focus_on_click(true);
    */

    if (maximise_)
    {
        maximize();
    }
    else
    {
        int _, w, h;
        view_->get_preferred_width(_, w);
        view_->get_preferred_height(_, h);
        set_default_size(w, h);
    }

    nvim_->io_error_signal().connect(
            sigc::mem_fun(*this, &Window::on_nvim_error));

    set_geometry_hints();

    present();
    nvim_->redraw_set_title.connect
        (sigc::mem_fun(this, &Window::on_redraw_set_title));
    bat_conn_.disconnect();
}

TabPage *Window::create_tab_page(const TabInfo &info, TabVector::iterator it)
{
    // page needs to be a pointer so we can copy it into the lambda
    auto page = new TabPage(view_, info);
    auto &label = page->get_label_widget();

    page->show_all();
    label.show_all();

    if (it == tabs_->end())
    {
        tabs_->emplace_back(page);
        notebook_->append_page(*page, label);
    }
    else
    {
        tabs_->emplace(it, page);
        notebook_->insert_page(*page, label, std::distance(tabs_->begin(), it));
    }

    bufs_and_tabs_.get_tab_title(info.handle, [page](const std::string &s)
    {
        page->set_label_text(s);
    });
    return page;
}

void Window::show_or_hide_tabs()
{
    bool show = Application::app_gsettings()->get_boolean("gui-tabs");
    show_or_hide_tabs(show);
    nvim_->nvim_set_option("showtabline", show ? 0 : show_tab_line_);
}

void Window::show_or_hide_tabs(bool show)
{
    notebook_->set_show_tabs(show);
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
