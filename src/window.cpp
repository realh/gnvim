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
    maximise_(maximise), columns_(width), lines_(height)
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
    for (auto page: pages_)
        delete page;
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
    add(*notebook_);
    notebook_->show_all();

    int pnum = 0;
    int current = 0;
    for (const auto &tab: bufs_and_tabs_.get_tabs())
    {
        create_tab_page(tab, pnum == 0);
        if (tab.handle == bufs_and_tabs_.get_current_tab_handle())
        {
            current = pnum;
        }
        ++pnum;
    }
    notebook_->signal_switch_page().connect([this](Gtk::Widget *w, int)
    {
        if (view_->get_current_widget() != w)
        {
            view_->set_current_widget(w);
        }
        if (ignore_switch_page_)
            return;
        const auto &handle = static_cast<TabPage *>(w)->get_vim_handle();
        if (handle != bufs_and_tabs_.get_current_tab_handle())
        {
            nvim_->nvim_set_current_tabpage(handle);
        }
    });

    notebook_->set_current_page(current);
    auto page = notebook_->get_nth_page(current);
    view_->set_current_widget(page);

    bufs_and_tabs_.signal_tab_enter().connect([this](int handle)
    {
        int cpn = notebook_->get_current_page();
        auto cp = (cpn != -1) ? static_cast<TabPage *>
                (notebook_->get_nth_page(cpn))
                : nullptr;
        if (cp && gint64(cp->get_vim_handle()) == handle)
        {
            g_debug("Handle %d is already current in GUI page %d",
                    handle, cpn);
            return;
        }

        unsigned n;
        for (n = 0; n < pages_.size(); ++n)
        {
            auto page = pages_[n];
            if (gint64(page->get_vim_handle()) == handle)
            {
                ++ignore_switch_page_;
                g_debug("Changing current GUI page from %d to %d for handle %d",
                        cpn, n, handle);
                notebook_->set_current_page(n);
                view_->set_current_widget(page);
                //nvim_->nvim_set_current_tabpage(handle);
                --ignore_switch_page_;
                break;
            }
        }
        if (n == pages_.size())
        {
            g_critical("Can't find nvim's current tab %d in notebook", handle);
        }
    });

    bufs_and_tabs_.signal_tabs_listed().connect
        (sigc::mem_fun(*this, &Window::on_tabs_listed));

    nvim_->start_ui(columns_, lines_);

    nvim_->io_error_signal().connect(
            sigc::mem_fun(*this, &Window::on_nvim_error));

    set_geometry_hints();

    present();
    nvim_->redraw_set_title.connect
        (sigc::mem_fun(this, &Window::on_redraw_set_title));
    bat_conn_.disconnect();
}

TabPage *Window::create_tab_page(const TabInfo &info, bool first)
{
    auto page = create_tab_page(info, -1);
    if (!first)
        return page;
    if (maximise_)
    {
        maximize();
    }
    else
    {
        Gtk::Requisition _, nbnat;
        notebook_->get_preferred_size(_, nbnat);
        //g_debug("Initial notebook size nat %dx%d", nbnat.width, nbnat.height);
        /*
        int _, pw, ph;
        view_->get_preferred_width(_, pw);
        view_->get_preferred_height(_, ph);
        g_debug("first page nat size %dx%d", pw, ph);
        */
        set_default_size(nbnat.width, nbnat.height);
    }
    return page;
}

TabPage *Window::create_tab_page(const TabInfo &info, int position)
{
    ++ignore_switch_page_;
    g_debug("create_tab: Inc ignore_switch to %d", ignore_switch_page_);

    g_debug("Creating new page for %ld at position %d",
            (gint64) info.handle, position);
    // page needs to be a pointer so we can copy it into the lambda
    TabPage *page;
    if (position == -1)
    {
        pages_.push_back(new TabPage(view_, info));
        page = pages_[pages_.size() - 1];
    }
    else
    {
        pages_.insert(pages_.begin() + position, new TabPage(view_, info));
        page = pages_[position];
    }
    auto &label = page->get_label_widget();
    notebook_->insert_page(*page, label, position);

    view_->set_current_widget(page);

    page->show_all();
    label.show_all();

    bufs_and_tabs_.get_tab_title(info.handle, [page](const std::string &s)
    {
        page->set_label_text(s);
    });

    --ignore_switch_page_;
    g_debug("create_tab: Dec ignore_switch to %d", ignore_switch_page_);

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

std::vector<TabPage *>::iterator
Window::find_page_with_handle(const VimTabpage &handle)
{
    return std::find_if(pages_.begin(), pages_.end(),
        [&handle](const TabPage *gtab)->bool
        {
            return gtab->get_vim_handle() == handle;
        });
}

void Window::on_tabs_listed(const std::vector<TabInfo> &tabv)
{
    ++ignore_switch_page_;
    g_debug("tabs_listed: Inc ignore_switch to %d", ignore_switch_page_);

    g_debug("on_tabs_listed tabv:");
    for (const auto &vtab: tabv)
    {
        g_debug("  handle %ld", (gint64) vtab.handle);
    }

    // First delete any GUI tabs without corresponding nvim tabs
    unsigned n = 0;
    while (n < pages_.size())
    {
        auto &handle = pages_[n]->get_vim_handle();
        if (std::find_if(tabv.begin(), tabv.end(),
            [&handle](const TabInfo &i)->bool
            {
                return i.handle == handle;
            }) == tabv.end())
        {
            g_debug("Removing tab handle %ld from position %d",
                    (gint64) handle, n);
            notebook_->remove_page(n);
            delete pages_[n];
            pages_.erase(pages_.begin() + n);
        }
        else
        {
            g_debug("Tab handle %ld at position %d", (gint64) handle, n);
            ++n;
        }
    }

    // Then create GUI tabs for any new nvim tabs
    n = 0;
    for (const auto &vtab: tabv)
    {
        if (find_page_with_handle(vtab.handle) == pages_.end())
        {
            create_tab_page(vtab, n >= pages_.size() ? -1 : (int) n);
        }
        ++n;
    }

    g_debug("pages_:");
    for (auto pg: pages_)
    {
        g_debug("  %ld", (gint64) pg->get_vim_handle());
    }

    // Reposition any tabs that have been reordered in nvim
    // TODO: When we support Notebook::page-reordered signal we should
    // suppress it temporarily here
    n = 0;
    for (const auto &vtab: tabv)
    {
        if (pages_[n]->get_vim_handle() != vtab.handle)
        {
            auto it = find_page_with_handle(vtab.handle);
            auto w = *it;
            g_debug("Moving %ld from %ld to %d",
                gint64(vtab.handle), it - pages_.begin(), n);
            std::swap(pages_[n], *it);
            notebook_->reorder_child(*w, n);
        }
        ++n;
    }

    --ignore_switch_page_;
    g_debug("tabs_listed: Dec ignore_switch to %d", ignore_switch_page_);
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
