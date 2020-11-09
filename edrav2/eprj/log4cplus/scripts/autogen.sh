#!/bin/sh

libtoolize --force --automake

autoheader
aclocal $ACLOCAL_FLAGS
autoconf
automake --ignore-deps --add-missing

echo 'run "configure; make"'

