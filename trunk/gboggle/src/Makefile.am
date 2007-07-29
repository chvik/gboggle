AM_CPPFLAGS = @PACKAGE_CFLAGS@
bin_PROGRAMS = gboggle #test
sources = board_widget.c board_widget.h \
	  board.c board.h \
	  boggle.h boggle.c \
	  ui.h ui.c \
	  langconf.h langconf.c
INCLUDES = -DSYSCONFDIR=\"@sysconfdir@\"
sysconf_DATA = gbogglerc

gboggle_SOURCES = $(sources) main.c
gboggle_LDADD = @PACKAGE_LIBS@

#test_SOURCES = $(sources) test.c
#test_LDADD = @PACKAGE_LIBS@

gbogglerc: gbogglerc.in
	sed s,'@DICTDIR@',@datadir@/dict, $< >$@
