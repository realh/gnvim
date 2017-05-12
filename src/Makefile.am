bin_PROGRAMS = gnvim

gnvim_SOURCES = \
	app.cpp			app.h			\
	bufsandtabs.cpp		bufsandtabs.h		\
	cell-attributes.cpp	cell-attributes.h	\
	dcs-dialog.cpp 		dcs-dialog.h		\
	defns.h						\
	ext-types.cpp		ext-types.h		\
	git-version.h					\
	main.cpp					\
	msgpack-adapter.cpp	msgpack-adapter.h	\
	msgpack-promise.h				\
	msgpack-rpc.cpp		msgpack-rpc.h		\
	nvim-bridge.cpp		nvim-bridge.h		\
	nvim-grid-view.cpp	nvim-grid-view.h	\
	request-set.cpp		request-set.h		\
	resources.cpp					\
	text-grid.cpp		text-grid.h		\
	text-grid-view.cpp	text-grid-view.h	\
	window.cpp		window.h

BUILT_SOURCES = git-version.h resources.cpp

git-version.h: $(top_srcdir)/.git
	echo '#ifndef GNVIM_GIT_VERSION' > $@
	echo '#define GNVIM_GIT_VERSION \
		"'`$(GIT_BINARY) describe 2>/dev/null`'"' >> $@
	echo '#endif' >> $@

GNVIM_UI_FILES = gnvim.gresource.xml appmenu.ui

resources.cpp: $(GNVIM_UI_FILES)
	glib-compile-resources $(srcdir)/gnvim.gresource.xml \
		--target=resources.cpp \
		--generate-source --sourcedir=$(srcdir)

gnvim_CXXFLAGS = $(gnvim_CFLAGS)
gnvim_LDFLAGS = $(gnvim_LIBS)

EXTRA_DIST = $(GNVIM_UI_FILES)
