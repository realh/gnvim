/* window.h
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

#ifndef GNVIM_WINDOW_H
#define GNVIM_WINDOW_H

#include "defns.h"

#include <memory>
#include <vector>

#include "bufsandtabs.h"
#include "nvim-bridge.h"
#include "nvim-grid-widget.h"
#include "request-set.h"
#include "tab-page.h"

namespace Gnvim
{

class Window : public Gtk::ApplicationWindow {
public:
    Window(bool maximise, int width, int height,
            std::shared_ptr<NvimBridge> nvim_);

    ~Window();

    void force_close();

    bool has_modifications() const
    {
        return bufs_and_tabs_.any_modified();
    }

    void nvim_discard_all(bool force = false)
    {
        if (force || nvim_->own_instance())
            nvim_->nvim_command("qa!");
    }

    void nvim_save_all(bool force_close = false)
    {
        if (force_close || nvim_->own_instance())
            nvim_->nvim_command("wqa!");
        else
            nvim_->nvim_command("wa!");
    }
protected:
    //virtual void on_size_allocate(Gtk::Allocation &alloc) override;

    virtual bool on_delete_event(GdkEventAny *) override;
private:
    using TabVector = std::vector<std::unique_ptr<TabPage>>;

    TabPage *create_tab_page(const TabInfo &info)
    {
        return create_tab_page(info, tabs_->end());
    }

    TabPage *create_tab_page(const TabInfo &info, TabVector::iterator it);

    void show_or_hide_tabs();

    void show_or_hide_tabs(bool show);

    void on_options_read(RequestSet *);

    void ready_to_start();

    void on_nvim_error(Glib::ustring desc);

    void on_redraw_set_title(const std::string &);

    // Geometry hints have a history of breakage and serve almost no useful
    // purpose in current desktops
    void set_geometry_hints() {}

    bool force_close_ {false};

    std::shared_ptr<NvimBridge> nvim_;
    BufsAndTabs bufs_and_tabs_;
    sigc::connection bat_conn_;

    /* gtkmm seems rather dodgy at handling contained widgets. AFAICT a GObject
     * gets destroyed with its parent or when removed from its parent, but
     * leaves the glibmm wrapper pointing to some sort of null object. It seems
     * safe to delete the glibmm pointer, but using RefPtr doesn't cause that
     * to happen.
     */
    NvimGridView *view_ {nullptr};
    NvimGridWidget *view_w_ {nullptr};
    Gtk::Notebook *notebook_;

    static RefPtr<Gio::ApplicationCommandLine> null_cl;

    bool maximise_;
    int columns_, lines_;
    int show_tab_line_;
    sigc::connection delete_event_conn_;

    TabVector *tabs_;
};

}

#endif // GNVIM_WINDOW_H
