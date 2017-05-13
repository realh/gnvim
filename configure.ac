#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gnvim], [0.0.3], [h@realh.co.uk])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/nvim-bridge.cpp])
AC_CONFIG_HEADERS([config.h])
#AC_CONFIG_MACRO_DIRS([m4])

GLIB_GSETTINGS

# Checks for programs.
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11
PKG_PROG_PKG_CONFIG
AX_WITH_PROG([GIT_BINARY], [git], [true])

# Checks for libraries.
PKG_CHECK_MODULES([gnvim], [gtkmm-3.0 msgpack])

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for library functions.

# Checks for typedefs, structures, and compiler characteristics.

# Other checks

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
