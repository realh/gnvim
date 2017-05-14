/* start-bundle.h
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

#ifndef GNVIM_START_BUNDLE_H
#define GNVIM_START_BUNDLE_H

#include "defns.h"

#include <string>
#include <vector>

namespace Gnvim
{

/** Bundles the data needed to start an nvim instance (command line,
 *  environment and cwd).
 *  ApplicationCommandLine isn't suitable because we want the command line
 *  after it's been processed for gnvim options.
 */
class StartBundle {
public:
    /// @tparam V   universal reference to std::vector<std::string>
    // /@tparam S   universal reference to std::string
    // /@tparam T   universal reference to std::string
    template<class V, class S, class T>
    StartBundle(char const * const *argv, int argc, V environ, S cwd,
            T init_file, bool full_command)
        : environ_(std::forward<V>(environ)),
          cwd_(std::forward<S>(cwd)),
          init_file_(std::forward<T>(init_file))
    {
        init(argv, argc, full_command);
    }

    StartBundle(const StartBundle &) = default;
    StartBundle(StartBundle &&) = default;
    StartBundle &operator=(const StartBundle &) = default;
    StartBundle &operator=(StartBundle &&) = default;

    const std::vector<std::string> &get_command_line() const
    {
        return cmd_line_;
    }

    const std::vector<std::string> &get_environ() const
    {
        return environ_;
    }

    const std::string &get_cwd() const
    {
        return cwd_;
    }
private:
    void init(char const * const *argv, int argc, bool full_command);

    std::vector<std::string> cmd_line_;
    std::vector<std::string> environ_;
    std::string cwd_;
    std::string init_file_;

    static std::vector<std::string> static_environ_;
};

}

#endif // GNVIM_START_BUNDLE_H
