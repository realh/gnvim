# gnvim

A GTK3 GUI for neovim.

## Status

At the moment this seems to be usable but doesn't yet offer anything that you
can't do with neovim in a terminal. The text view widget has just been
completely rewritten with a custom drawing system using Cairo/Pango directly
because using GtkTextView in this way was too inefficient. That's not to say
that GtkTextView is inefficient in general, but it was just never designed to
manage a viewport by moving text around instead of using standard scrollbars.

## Features currently implemented

* Configurable GUI

* Multiple windows, one nvim instance per window

* Flashing cursor (optional, using gsettings), caret style in insert mode

* Configurable cursor colour

* Detect unsaved files before closing

## High priority future features

* Use options from the vim instance where possible

* Preferences dialog

* Optional GUI tab handling

* Options to connect to remote nvim instances

* Share state between nvim instances

## Slightly lower priority future features

Not all of these may be possible.

* Windows binary

* Mac support (cmd key)

* System clipboard integration with vim's + register (and Mac cmd key)

* Better support for "smooth" touchpad scrolling

* Scrollbars (one for each vim split window ie multiple per GUI window) might
  even be possible

## Building and installing

### Build requirements

* [gtkmm](http://www.gtkmm.org) version 3.22 or later

* [msgpack C/C++ library](https://github.com/msgpack/msgpack-c) (version 1.x)

* An up-to-date GNU Build System (autotools)

### Building

Use the autotools; a CMakeLists.txt file is also supplied, but this is only to
make incremental development builds more convenient (and may also be useful for
MS Windows). Only the autotools support full installation on Unix platforms.

Run `autoreconf -i` in the top-level source folder to bootstrap the build, then
run `./configure`, `make` and `sudo make install` as usual.

## Configuring the GUI

Until the preferences dialog is implemented use dconf-editor. gnvim's options
have the path `/uk/co/realh/gnvim/`.

## Issues

### New windows sometimes stay blank

This is usually caused by trying to load a file for which a vim swap file,
exists, an error in `init.nvim` or an invalid command line. Unfortunately
these issues cause neovim to stall on waiting for input before allowing a
remote UI to attach. See
[neovim issue 3901](https://github.com/neovim/neovim/issues/3901).

Close the window and run nvim in a terminal to deal with
the situation.
