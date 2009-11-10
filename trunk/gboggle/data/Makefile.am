sysconf_DATA = gbogglerc
desktopentry_DATA = gboggle.desktop

gtkrc_DATA = gtkrc

EXTRA_DIST = gboggle.48.png gboggle.64.png gboggle.svg gbogglerc.in gboggle.desktop

CLEANFILES = $(sysconf_DATA) $(desktopentry_DATA)

gboggle.desktop: gboggle.desktop.in
	sed s,'@'prefix'@',@prefix@, $< >$@

gbogglerc: gbogglerc.in
	sed s,'@'dictdir'@',@datadir@/gboggle/dict, $< >$@

ICON_PREFIX=$(DESTDIR)$(icondir)

install-data-local:
	mkdir -p $(ICON_PREFIX)/48x48/$(iconsubdir)
	$(INSTALL_DATA) gboggle.48.png $(ICON_PREFIX)/48x48/$(iconsubdir)/gboggle.png
	mkdir -p $(ICON_PREFIX)/64x64/$(iconsubdir)
	$(INSTALL_DATA) gboggle.64.png $(ICON_PREFIX)/64x64/$(iconsubdir)/gboggle.png
	mkdir -p $(ICON_PREFIX)/scalable/$(iconsubdir)
	$(INSTALL_DATA) gboggle.svg $(ICON_PREFIX)/scalable/$(iconsubdir)/gboggle.svg
