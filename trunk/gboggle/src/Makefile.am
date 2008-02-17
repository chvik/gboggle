AM_CPPFLAGS = @PACKAGE_CFLAGS@
bin_PROGRAMS = gboggle
sources = board_widget.c board_widget.h \
	  board.c board.h \
	  boggle.h boggle.c \
	  ui.h ui.c \
	  langconf.h langconf.c
INCLUDES = -DSYSCONFDIR=\"@sysconfdir@\"
sysconf_DATA = gbogglerc

gboggle_SOURCES = $(sources) main.c
gboggle_LDADD = @PACKAGE_LIBS@ $(LIBINTL)

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

gbogglerc: gbogglerc.in
	sed s,'@DICTDIR@',@datadir@/dict, $< >$@
