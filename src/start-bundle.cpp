/* start-bundle.cpp
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

#include "start-bundle.h"

namespace Gnvim
{

void StartBundle::init(char const * const *argv, int argc, bool full_command)
{
    cmd_line_.clear();
    if (!full_command)
    {
        cmd_line_.push_back("nvim");
        // Check whether -u or --embed are already present
        bool u = false, embed = false;
        for (int n = 1; n < argc; ++n)
        {
            if (!strcmp(argv[n], "-u"))
                u = true;
            else if (!strcmp(argv[n], "--embed"))
                embed = true;
        }

        if (!embed)
            cmd_line_.push_back("--embed");

        // Load custom init file if present and -u wasn't overridden
        if (!u && init_file_.size())
        {
            cmd_line_.push_back("-u");
            cmd_line_.push_back(init_file_);
        }
    }
    /*
    std::ostringstream s;
    for (const auto &a: cmd_line_)
    {
        s << a << ' ';
    }
    g_debug("%s", s.str().c_str());
    */

    for (int n = 1; n < argc; ++n)
    {
        cmd_line_.push_back(argv[n]);
    }

    //modify_env(environ, "XDG_CONFIG_HOME", Glib::get_user_config_dir(), false);
    //modify_env(environ, "XDG_DATA_HOME", Glib::get_user_data_dir(), false);
    /*
    std::ostringstream s;
    for (const auto &a: environ)
    {
        s << a << std::endl;
    }
    g_debug("%s", s.str().c_str());
    */

    // Only the first call to Application::on_command_line has an enviroment,
    // on subsequent calls it's empty. glib(mm) bug?
    if (environ_.size() && !static_environ_.size())
        static_environ_ = environ_;
    else if (!environ_.size() && static_environ_.size())
        environ_ = static_environ_;
    for (int n = 0; n < argc; ++n)
        cmd_line_.push_back(argv[n]);
}

std::vector<std::string> StartBundle::static_environ_;

}
