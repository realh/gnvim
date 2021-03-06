/* app.cpp
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

#include <cstdio>
#include <cstring>

#include "app.h"
#include "dcs-dialog.h"

// CMake defines the variable on the command line, automake generates a file
#ifdef HAVE_CONFIG_H
#include "git-version.h"
#endif

#include "window.h"

namespace Gnvim
{

Application::Application()
        : Glib::ObjectBase(typeid(Application)),
        Gtk::Application("uk.co.realh.gnvim",
            Gio::APPLICATION_HANDLES_COMMAND_LINE),
        options_("NVIM_ARGUMENTS"),
        main_opt_group_("gnvim", _("gnvim options")),
        prop_dark_theme_(*this, "dark-theme", false, _("Use dark theme"),
            _("Whether to use the dark theme"), Glib::PARAM_READWRITE)
{
    options_.set_summary(_("Run neovim with a GUI"));
    options_.set_description(
            _("Other options are passed on to the embedded nvim instance;\n"
                "--embed is added automatically unless --socket "
                "or --command are used"));
    options_.set_help_enabled(true);
    options_.set_ignore_unknown_options(true);

    Glib::OptionEntry entry;
    entry.set_long_name("socket");
    entry.set_flags(Glib::OptionEntry::FLAG_IN_MAIN);
    entry.set_description(_("Connect to nvim over a socket.\n"
                "                                        "
                "If ADDR begins with '/' it's a\n"
                "                                        "
                "Unix socket, otherwise TCP"));
    entry.set_arg_description(_("ADDR"));
    main_opt_group_.add_entry(entry, opt_socket_);

    entry = Glib::OptionEntry();
    entry.set_long_name("geometry");
    entry.set_short_name('g');
    entry.set_flags(Glib::OptionEntry::FLAG_IN_MAIN);
    entry.set_description(_("Set editor size in chars WIDTHxHEIGHT\n"
                "                                        or maximised"));
    entry.set_arg_description(_("WIDTHxHEIGHT | max"));
    main_opt_group_.add_entry(entry,
            sigc::mem_fun(this, &Application::on_opt_geometry));

    entry = Glib::OptionEntry();
    entry.set_long_name("help-nvim");
    entry.set_flags(Glib::OptionEntry::FLAG_IN_MAIN);
    entry.set_description(_("Show options handled by embedded nvim"));
    main_opt_group_.add_entry(entry, opt_help_nvim_);

    entry = Glib::OptionEntry();
    entry.set_long_name("command");
    entry.set_flags(Glib::OptionEntry::FLAG_IN_MAIN);
    entry.set_description(_("Subsequent arguments form the full\n"
                "                                        "
                "nvim command"));
    main_opt_group_.add_entry(entry, opt_command_);

    options_.set_main_group(main_opt_group_);
    g_option_context_add_group(options_.gobj(), gtk_get_option_group(0));

    property_dark_theme().signal_changed().connect
        (sigc::mem_fun(this, &Application::on_prop_dark_theme_changed));
    app_gsettings()->bind("dark", property_dark_theme());
}

int Application::on_command_line(const RefPtr<Gio::ApplicationCommandLine> &cl)
{
    int argc;
    char **argv = cl->get_arguments(argc);
    std::unique_ptr<char *, void(*)(char **)> scoped_argv(argv, g_strfreev);

    auto settings = app_gsettings();
    opt_max_ = settings->get_boolean("maximise");
    opt_width_ = -1;
    opt_height_ = -1;
    opt_help_nvim_ = false;
    opt_socket_.clear();
    opt_command_ = false;
    if (!options_.parse(argc, argv))
        return 1;
    if (opt_help_nvim_)
    {
        Glib::spawn_command_line_sync("nvim --help");
        return 0;
    }
    try
    {
        auto nvim = std::make_shared<NvimBridge>();
        if (opt_socket_.size())
        {
            nvim->start(opt_socket_);
        }
        else
        {
            StartBundle sb(argv, argc, cl->get_environ(), cl->get_cwd(),
                    app_gsettings_->get_string("init-file"), opt_command_);
            nvim->start(sb);
        }
        auto win = new Window(opt_max_, opt_width_, opt_height_, nvim);
        add_window(*win);
    }
    catch(Glib::Exception &e)
    {
        g_warning("Failed to start new nvim instance: %s", e.what().c_str());
        return 1;
    }
    catch(std::exception &e)
    {
        g_warning("Failed to start new nvim instance: %s", e.what());
        return 1;
    }
    return 0;
}

void Application::on_startup()
{
    Gtk::Application::on_startup();

    Gtk::Window::set_default_icon_name("gnvim");

    add_action("about", sigc::mem_fun(*this, &Application::on_action_about));
    add_action("quit", sigc::mem_fun(*this, &Application::on_action_quit));

    auto builder = Gtk::Builder::create_from_resource
        ("/uk/co/realh/gnvim/appmenu.ui");
    set_app_menu(RefPtr<Gio::MenuModel>::cast_static
            (builder->get_object("appmenu")));
}

void Application::on_window_removed(Gtk::Window *win)
{
    Gtk::Application::on_window_removed(win);
    // We created window with new, so we also have to delete it
    delete win;
    if (get_windows().size() < 1)
    {
        g_debug("No open windows, quitting");
        //quit();
    }
    else
    {
        g_debug("Window removed, but %ld left", get_windows().size());
    }
}

template<class T> bool Application::foreach_window
(std::vector<Gtk::Window *> &wins, T functor)
{
    bool result = false;
    for (auto &win: wins)
    {
        if (functor(static_cast<Window *>(win)))
        {
            result = true;
            break;
        }
    }
    return result;
}

bool Application::on_opt_geometry(const Glib::ustring &,
        const Glib::ustring &geom, bool has_value)
{
    if (!has_value)
    {
        g_critical("--geometry needs an argument");
        return false;
    }

    if (geom.find("max") == 0)
    {
        opt_max_ = true;
        return true;
    }
    else
    {
        int w = 0, h = 0;
        if (sscanf(geom.c_str(), "%dx%d", &w, &h) != 2
                || w <= 0 || h <= 0)
        {
            g_critical("--geometry argument must be WxH or 'max'");
            return false;
        }
        opt_width_ = w;
        opt_height_ = h;
    }

    return true;
}

// Use lazy init for GSettings, apparently can't do C++ static init of GObjects

RefPtr<Gio::Settings> Application::app_gsettings()
{
    if (!app_gsettings_)
        app_gsettings_ = Gio::Settings::create("uk.co.realh.gnvim");
    return app_gsettings_;
}

RefPtr<Gio::Settings> Application::sys_gsettings()
{
    if (!sys_gsettings_)
        sys_gsettings_ = Gio::Settings::create("org.gnome.desktop.interface");
    return sys_gsettings_;
}

void Application::on_prop_dark_theme_changed()
{
    auto gsettings = Gtk::Settings::get_default();
    gsettings->property_gtk_application_prefer_dark_theme().set_value
            (prop_dark_theme_.get_value());
}

void Application::on_action_about()
{
    if (about_dlg_)
    {
        about_dlg_->present();
        return;
    }

    Gtk::AboutDialog about;
    about_dlg_ = &about;

    std::string version = PACKAGE_VERSION;
    std::string gversion = GNVIM_GIT_VERSION;
    // Comparing the 2 macros with strcmp causes a warning, but I'm not sure
    // whether identical string literals are guaranteed to share the same
    // address, so wrap both in strings even though one might be redundant.
    if (gversion.length() && version != gversion)
        version += std::string(" (") + gversion + ')';
    about.set_version(version);

    about.set_copyright("\u00a9 2017 Tony Houghton");
    about.set_comments(_("A simple but productive GUI for neovim"));
    about.set_license_type(Gtk::LICENSE_GPL_3_0);
    about.set_website("https://github.com/realh/gnvim");
    about.set_logo_icon_name("gnvim");

    // gtkmm's object lifecycle management seems to break down with dialogs
    // showing without using run(). So we might as well make it modal and get
    // rid of the messages complaining about it not having a transient parent
    // But even when it's modal the app menu items can still be activated, so
    // we have to take that into account too.
    auto wins = get_windows();
    if (wins.size())
    {
        // Find currently focused window, otherwise just use first in list
        Gtk::Window *current = nullptr;
        if (!foreach_window(wins, [&current](Window *w)->bool
        {
            if (w->has_toplevel_focus())
            {
                current = w;
                return true;
            }
            return false;
        }))
        {
            current = wins[0];
        }
        about.set_transient_for(*current);
    }
    about.set_modal();

    about.present();
    about.run();
    about_dlg_ = nullptr;
}

void Application::on_action_quit()
{
    if (about_dlg_)
        about_dlg_->close();

    auto wins = get_windows();
    if (foreach_window(wins,
                [](Window *win)->bool { return win->has_modifications(); }))
    {
        DcsDialog dcs(*(wins[0]));
        switch (dcs.show_and_run())
        {
            case DcsDialog::DISCARD:
                foreach_window(wins, [](Window *win)->bool
                {
                    win->nvim_discard_all();
                    win->force_close();
                    return false;
                });
                quit();
                break;
            case DcsDialog::SAVE:
                foreach_window(wins, [](Window *win)->bool
                    {
                        win->nvim_save_all();
                        win->force_close();
                        return false;
                    });
                quit();
                break;
            default:
                break;
        }
    }
    else
    {
        foreach_window(wins, [](Window *win)->bool
        {
            win->force_close();
            return false;
        });
        quit();
    }
}

RefPtr<Gio::Settings> Application::app_gsettings_;
RefPtr<Gio::Settings> Application::sys_gsettings_;

}
