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

gboggle_SOURCES = $(sources)
gboggle_LDADD = @PACKAGE_LIBS@ @MAEMO_LIBS@ $(LIBINTL)


localedir = $(datadir)/locale

DEFS = -DLOCALEDIR=\"$(localedir)\" -DGTKRCFILE=\"$(gtkrcdir)/gtkrc\" @DEFS@
