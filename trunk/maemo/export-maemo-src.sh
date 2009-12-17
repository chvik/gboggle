#!/bin/sh

if [ "x$1" = x ]; then
    echo Usage: $0 targetdir
    exit 1
fi

scriptdir=`dirname $0`
targetdir=$1
debiandir="$scriptdir/debian"
sourcedir="$scriptdir/../gboggle"

svn export "$sourcedir" "$targetdir" && \
rm -rf "$targetdir/debian" && \
svn export "$debiandir" "$targetdir/debian" && \
echo maemo source exported to "$targetdir"
