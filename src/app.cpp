/* app.cpp
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

#include <cstdio>

#include "app.h"
#include "window.h"

namespace Gnvim
{

Application::Application ()
        : Gtk::Application ("uk.co.realh.gnvim",
            Gio::APPLICATION_HANDLES_COMMAND_LINE),
        options_("NVIM_ARGUMENTS"),
        main_opt_group_ ("gnvim", _("gnvim options")),
        gsettings_ (Gio::Settings::create ("uk.co.realh.gnvim"))
{
    options_.set_summary (_("Run neovim with a GUI"));
    options_.set_description (
            _("Other options are passed on to the embedded nvim instance;\n"
                "--embed is added automatically"));
    options_.set_help_enabled (true);
    options_.set_ignore_unknown_options (true);

    Glib::OptionEntry entry;
    entry.set_long_name ("geometry");
    entry.set_short_name ('g');
    entry.set_flags (Glib::OptionEntry::FLAG_IN_MAIN);
    entry.set_description (_("Set editor size in chars WIDTHxHEIGHT\n"
                "                                        or maximised"));
    entry.set_arg_description (_("WIDTHxHEIGHT | max"));
    main_opt_group_.add_entry (entry,
            sigc::mem_fun (this, &Application::on_opt_geometry));

    entry = Glib::OptionEntry ();
    entry.set_long_name ("help-nvim");
    entry.set_flags (Glib::OptionEntry::FLAG_IN_MAIN);
    entry.set_description (_("Show options handled by embedded nvim"));
    main_opt_group_.add_entry (entry, opt_help_nvim_);

    options_.set_main_group (main_opt_group_);
    g_option_context_add_group (options_.gobj (), gtk_get_option_group (0));
}

void Application::on_activate ()
{
    open_window (Glib::get_current_dir (), 0, nullptr);
}

int Application::on_command_line (const RefPtr<Gio::ApplicationCommandLine> &cl)
{
    int argc;
    char **argv = cl->get_arguments (argc);
    opt_max_ = gsettings_->get_boolean ("maximise");
    opt_width_ = gsettings_->get_uint ("columns");
    opt_height_ = gsettings_->get_uint ("lines");
    opt_help_nvim_ = false;
    if (!options_.parse (argc, argv))
        return 1;
    if (opt_help_nvim_)
    {
        Glib::spawn_command_line_sync ("nvim --help");
        return 0;
    }
    if (!open_window (cl->get_cwd (), argc, argv))
    {
        g_warning ("Failed to open window, exiting this instance");
        return 1;
    }
    return 0;
}

bool Application::open_window (const std::string &cwd, int argc, char **argv)
{
    try
    {
        Glib::ustring init_file = gsettings_->get_string ("init-file");
        auto win = new Window (opt_max_, opt_width_, opt_height_, init_file,
                cwd, argc, argv);
        g_debug ("Adding window");
        add_window (*win);
        g_debug ("Window added, now have %ld", get_windows ().size ());
        return true;
    }
    catch (Glib::Exception &e)
    {
        g_warning ("Failed to start new nvim instance: %s", e.what ().c_str());
    }
    catch (std::exception &e)
    {
        g_warning ("Failed to start new nvim instance: %s", e.what ());
    }
    return false;
}

void Application::on_window_removed (Gtk::Window *)
{
    // Apparently this is called before the window is actually removed, so
    // size still includes the window being removed
    if (get_windows ().size () <= 1)
    {
        g_debug ("No open windows, quitting");
        quit ();
    }
    else
    {
        g_debug ("Window removed, but %ld left", get_windows ().size ());
    }
}

bool Application::on_opt_geometry (const Glib::ustring &,
        const Glib::ustring &geom, bool has_value)
{
    if (!has_value)
    {
        g_critical ("--geometry needs an argument");
        return false;
    }

    if (geom.find ("max") == 0)
    {
        opt_max_ = true;
        return true;
    }
    else
    {
        int w = 0, h = 0;
        if (sscanf (geom.c_str (), "%dx%d", &w, &h) != 2
                || w <= 0 || h <= 0)
        {
            g_critical ("--geometry argument must be WxH or 'max'");
            return false;
        }
        opt_width_ = w;
        opt_height_ = h;
    }

    return true;
}

}
