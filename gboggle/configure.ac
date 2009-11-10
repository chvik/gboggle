#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([gboggle],[1.1.2],[ch@elte.hu])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION) 
#AC_CONFIG_SRCDIR([config.h.in])
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_SUBST(CFLAGS)

# Checks for libraries.
PKG_CHECK_MODULES(PACKAGE, gtk+-2.0 gconf-2.0)

# Checks platform
PKG_CHECK_MODULES(MAEMO, hildon-1, [detected_maemo=yes],[detected_maemo=no])
AC_ARG_WITH(maemo,
    AC_HELP_STRING([--with-maemo], [Compiling for maemo platform]),
    [],
    [if test "$detected_maemo" = yes; then
         with_maemo=yes
     fi])
     
if test "${with_maemo}" = yes; then
    AC_MSG_NOTICE([Compiling for maemo platform])
    AC_DEFINE([HAVE_MAEMO], [1], [Define if you want build for maemo])
    desktopentrydir=`pkg-config osso-af-settings --variable=desktopentrydir`
    icondir="$datadir/icons/hicolor/scalable/hildon"
else
    desktopentrydir="$datadir/applications"
    icondir="$datadir/icons/hicolor/64x64/apps"
fi

gtkrcdir="$datadir/gboggle"
dictdir="$datadir/gboggle/dict"

AC_SUBST(desktopentrydir)
AC_SUBST(icondir)
AC_SUBST(gtkrcdir)
AC_SUBST(dictdir)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([pow])

# i18n support from GNU gettext.
AM_GNU_GETTEXT_VERSION([0.16.1])
AM_GNU_GETTEXT([external])


AC_CONFIG_FILES([Makefile po/Makefile.in
                 src/Makefile
                 data/Makefile
                 dict/Makefile
                ])
AC_OUTPUT
