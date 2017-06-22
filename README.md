# gnvim

A GTK3 GUI for neovim.

## Status

At the moment this seems to be usable but doesn't yet offer anything much that
you can't do with neovim in a terminal. The text view widget has just been
completely rewritten with a custom drawing system using Cairo/Pango directly
because using GtkTextView in this way was too inefficient. That's not to say
that GtkTextView is inefficient in general, but it was just never designed to
manage a viewport by moving text around instead of using standard scrollbars.

Gnvim has currently only been tested with neovim 0.1.7. I want to continue to
support this version for a while because it will remain the stable version in a
number of Linux distros. I will test with 0.2.x once official Debian packages
are available and start exploiting some new features such as its improved GUI
cursor support, hopefully retaining backwards compatibility with 0.1.7.

## Features currently implemented

* Configurable GUI

* Multiple windows, one nvim instance per window

* Flashing cursor (optional, using gsettings), caret style in insert mode

* Configurable cursor colour

* Detect unsaved files before closing

* Options to connect to remote nvim instances

* Shared state between nvim instances

## Partly implemented features, need work

* Optional GUI tab handling

* Use options from the vim instance where possible

## High priority future features

* Preferences dialog

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

## Usage hints

### Remote editing

Besides using the `--socket` option you can also edit over ssh by using the
`--command` option, for example:

    gnvim --command ssh remote.host nvim --embed -u ~/.config/nvim/gnvim.vim

Note that without options gnvim usually adds `--embed` automatically, and also
`-u ...` if a file has been specified in its gsettings, but `--command`
disables that behaviour so you will generally need to include `--embed`
yourself. And gnvim is, of course, unable to apply `--embed` or `-u` when using
`--socket` to connect to an nvim instance started elswhere.

### Tabs

gnvim can now manage tabs in the GUI with a GtkNotebook instead of leaving nvim
to draw the tabline in text. This is enabled with the `gui-tabs` gsetting.

#### Tab shortcuts

* Right-click or ctrl-click a tab close icon to close all other tabs.

* Shift-click to force close tabs with unsaved buffers.

More useful shortcuts will be added in the future.

#### Tab caveats

The following issues are mostly due to nvim not yet providing explicit support
for GUI tabs so I've had to use some hacks. nvim will have better support for
this feature in a future version, so I'll address them then.

* gnvim overrides vim's `showtabline` option when GUI tabs are enabled. Do not
  try to change the option in vim in this case. The overriden value is 2 rather
  than 0 as you might expect:- gnvim then excludes the tabline from its display
  and detects attempts to change the line as a way of telling when tabs might
  have been reordered.

* Clicking a tab close icon doesn't immediately close a tab, but simply sends a
  `tabclose` (or `tabonly`) command to nvim, which will refuse to close tabs if
  it would result in data loss. In gnvim it doesn't show feedback, so it will
  just appear as if the click was ignored.

* There's a remote possibility of the ordering of tabs in the GUI getting out
  of sync with nvim, but this is very unlikely unless you use vim's `:tabmove`
  command while its `showtabline` option is 0 and GUI tabs are disabled.

## Other issues

### New windows sometimes stay blank

This is usually caused by trying to load a file for which a vim swap file,
exists, an error in `init.nvim` or an invalid command line. Unfortunately
these issues cause neovim to stall on waiting for input before allowing a
remote UI to attach. See
[neovim issue 3901](https://github.com/neovim/neovim/issues/3901).

Close the window and run nvim in a terminal to deal with the situation.
