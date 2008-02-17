#!/bin/sh

autopoint -f
aclocal
autoheader
automake --copy --add-missing
autoconf
