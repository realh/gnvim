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
#include "request-set.h"

namespace Gnvim
{

class Window : public Gtk::ApplicationWindow {
public:
    Window(bool maximise, int width, int height, const std::string &init_file,
            RefPtr<Gio::ApplicationCommandLine> cl);

    ~Window();

    void force_close();

    bool has_modifications() const
    {
        return bufs_and_tabs_.any_modified();
    }
protected:
    //virtual void on_size_allocate(Gtk::Allocation &alloc) override;

    virtual bool on_delete_event(GdkEventAny *) override;
private:
    void ready_to_start(RequestSet *);

    void on_nvim_error(Glib::ustring desc);

    void on_redraw_set_title(const std::string &);

    void on_columns_response(const msgpack::object &o);

    void on_lines_response(const msgpack::object &o);

    // Geometry hints have a history of breakage and serve almost no useful
    // purpose in current desktops
    void set_geometry_hints() {}

    bool force_close_ {false};

    NvimBridge nvim_;
    BufsAndTabs bufs_and_tabs_;
    sigc::connection bat_conn_;

    // This is just a convenience pointer, life cycle is managed by GTK
    NvimGridView *view_;

    static RefPtr<Gio::ApplicationCommandLine> null_cl;

    bool maximise_;
    int columns_, lines_;
    std::unique_ptr<RequestSet> rqset_;
    sigc::connection delete_event_conn_;
};

}

#endif // GNVIM_WINDOW_H
