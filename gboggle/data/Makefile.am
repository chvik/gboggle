sysconf_DATA = gbogglerc
desktopentry_DATA = gboggle.desktop

gtkrc_DATA = gtkrc

icon_DATA = gboggle.png

EXTRA_DIST = $(icon_DATA) gbogglerc.in

CLEANFILES = $(sysconf_DATA) $(desktopentry_DATA)

gboggle.desktop: gboggle.desktop.in
	sed s,'@'prefix'@',@prefix@, $< >$@

gbogglerc: gbogglerc.in
	sed s,'@'dictdir'@',@datadir@/gboggle/dict, $< >$@
