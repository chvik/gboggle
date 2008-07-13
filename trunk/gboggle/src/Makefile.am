AM_CPPFLAGS = @PACKAGE_CFLAGS@ @MAEMO_CFLAGS@
bin_PROGRAMS = gboggle
sources = board_widget.c board_widget.h \
	  board.c board.h \
	  boggle.h boggle.c \
	  ui.h ui.c \
	  langconf.h langconf.c \
	  appdata.h \
	  util.h \
	  util.c \
	  main.c
INCLUDES = -DSYSCONFDIR=\"@sysconfdir@\"
sysconf_DATA = gbogglerc
desktopentry_DATA = gboggle.desktop
icon_DATA = gboggle.png

gboggle_SOURCES = $(sources)
gboggle_LDADD = @PACKAGE_LIBS@ @MAEMO_LIBS@ $(LIBINTL)

EXTRA_DIST = $(icon_DATA) gbogglerc.in
CLEANFILES = $(sysconf_DATA) $(desktopentry_DATA)

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

gbogglerc: gbogglerc.in
	sed s,'@DICTDIR@',@datadir@/dict, $< >$@
