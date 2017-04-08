# gnvim

A GTK3 GUI for neovim.

## Status

I'm not sure how far I can take this project, because neovim poses big
difficulties when trying to implement the sort of GUI I want for an all-purpose
text editor. But when thinking about how to solve the current problem (using
GtkTextView like this is too inefficient for scrolling etc) I've realised it
might even be possible to implement correctly-behaving scrollbars eventually,
so I'm going to persist with development for now.

## Features currently implemented

* Configurable GUI

* Multiple windows, one nvim instance per window

## High priority future features

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

* Scrollbars (one for each vim split window ie multiple per GU window) might
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
exists, a mistake in `init.nvim` or an invalid command line. Unfortunately
these issues cause neovim to stall waiting for input before allowing a remote
UI to attach. Close the window and run nvim in a terminal to deal with the
situation.

### Various redraw glitches

I'm not really sure whether this is the fault of gnvim or nvim. The source for
the [python-gui](https://github.com/neovim/python-gui) mentions redraw glitches
but also claims to have a workaround I may be able to copy. For now press
`Ctrl-L` to refresh the view.

### The cursor is the wrong colour/hard to see in insert mode

gnvim doesn't (currently) provide options for this, but you can configure it by
adding something like the following to `gtk.css`, usually found in
`~/.config/gtk-3.0/`:

```css
textview.gnvim {
    -GtkWidget-cursor-aspect-ratio: 0.12;
    caret-color: #ef2929;
}
```

## Other technical points of interest

gnvim uses GtkTextView to display the text, with a GtkTextBuffer that shadows
the currently displayed vim text and highlights. It's a rather novel way of
using GtkTextView, but it's easy and at first it seemed ideal for the job. But
in practice it works out to very inefficient, and fixing that would require
nearly as much work as writing a specialised widget.
