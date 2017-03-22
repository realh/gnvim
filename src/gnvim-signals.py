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

def const_ref (a):
    if a == "int":
        return a
    else:
        return "const " + a + " &"

def list_signal (mname, name, args):
    global signals_list
    if sys.argv[1].startswith ("def"):
        if not args:
            sig_template_args = "void"
        else:
            sig_template_args = 'void, ' \
            + ', '.join (const_ref (a) for a in args)
        signals_list += "    sigc::signal<%s> nvim_%s;\n" \
        % (sig_template_args, name)
    elif sys.argv[1].startswith ("reg"):
        if not args:
            class_template_args = "void"
        else:
            class_template_args = ', '.join (const_ref (a) for a in args)
        signals_list += \
                ('    %s.emplace ("%s",\n' \
                + '        new MsgpackAdapter<%s> (nvim_%s));\n') \
                % (mname, name, class_template_args, name)

# Redraw methods
def r (s):
    if ',' in s:
        name, args = s.split(', ', 1)
        args = args.split(', ')
    else:
        name = s
        args = None
    list_signal ("redraw_adapters_", name, args)
                
r ("resize, int, int")
r ("clear")
r ("eol_clear")
r ("cursor_goto, int, int")
r ("update_fg, int")
r ("update_bg, int")
r ("update_sp, int")
r ("highlight_set, msgpack::object")
r ("put, msgpack::object_array")
r ("set_scroll_region, int, int, int, int")
r ("scroll, int")
r ("set_title, std::string")
r ("set_icon, std::string")
r ("mouse_on")
r ("mouse_off")
r ("busy_on")
r ("busy_off")
r ("suspend")
r ("bell")
r ("visual_bell")
r ("update_menu")
r ("mode_change, std::string")
r ("popupmenu_show, msgpack::object, int, int, int")
r ("popupmenu_select, int")
r ("popupmenu_hide")

code = open (sys.argv[2], 'r').read ()
code = code.replace("#define GNVIM_SIGNALS\n", signals_list)
open (sys.argv[3], 'w').write (code)
