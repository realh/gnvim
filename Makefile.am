# ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

gsettings_SCHEMAS = uk.co.realh.gnvim.gschema.xml
EXTRA_DIST = $(gsettings_SCHEMAS)

@GSETTINGS_RULES@

appicondir = $(datadir)/icons/hicolor/scalable/apps
appicon_DATA = gnvim.svg

desktopdir = $(datadir)/applications
desktop_DATA = gnvim.desktop
