#!/usr/bin/env python3
# nvim-signals.py
#
# Copyright (C) 2017 Tony Houghton
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This either declares the signals in NvimBridge's class definition, or stores
# them in the maps in map_adapters().

import sys

signals_list = ""

def list_signal (mname, skip0, name, args):
    global signals_list
    if sys.argv[1].startswith ("def"):
        if len(args) == 1 and args[0] == "void":
            sig_template_args = "void"
        else:
            sig_template_args = ', '.join ("const %s &" % a for a in args)
        signals_list += "    sigc::signal<void, %s> nvim_%s;\n" \
        % (sig_template_args, name)
    elif sys.argv[1].startswith ("reg"):
        class_template_args = ', '.join (args)
        signals_list += \
                ('    %s.emplace ("%s",\n' \
                + '        MsgpackAdapter<%s> (nvim_%s, %s));\n') \
                % (mname, name, class_template_args, name, skip0)

# Redraw methods
def r (s):
    name, args = s.split(', ', 1)
    args = args.split(', ')
    list_signal ("redraw_adapters_", "true", name, args)
                
r ("resize, int, int")
r ("clear, void")
r ("eol_clear, void")
r ("cursor_goto, int, int")
r ("update_fg, int")
r ("update_bg, int")
r ("update_sp, int")
r ("highlight_set, msgpack::object")
r ("put, std::string")
r ("set_scroll_region, int, int, int, int")
r ("scroll, int")
r ("set_title, std::string")
r ("set_icon, std::string")
r ("mouse_on, void")
r ("mouse_off, void")
r ("busy_on, void")
r ("busy_off, void")
r ("suspend, void")
r ("bell, void")
r ("visual_bell, void")
r ("update_menu, void")
r ("mode_change, std::string")
r ("popupmenu_show, msgpack::object, int, int, int")
r ("popupmenu_select, int")
r ("popupmenu_hide, void")

code = open (sys.argv[2] + ".in", 'r').read ()
code = code.replace("#define GNVIM_SIGNALS\n", signals_list)
open (sys.argv[2], 'w').write (code)
