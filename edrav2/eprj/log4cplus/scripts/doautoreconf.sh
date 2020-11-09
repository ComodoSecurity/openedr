#!/bin/sh

export AUTOMAKE_SUFFIX=-1.16.1
export AUTOCONF_SUFFIX=-2.69
export LIBTOOL_SUFFIX=-2.4.6

export ACLOCAL="aclocal${AUTOMAKE_SUFFIX}"
export AUTOMAKE="automake${AUTOMAKE_SUFFIX}"

export AUTOCONF="autoconf${AUTOCONF_SUFFIX}"
export AUTOHEADER="autoheader${AUTOCONF_SUFFIX}"
export AUTOM4TE="autom4te${AUTOCONF_SUFFIX}"
export AUTORECONF="autoreconf${AUTOCONF_SUFFIX}"
export AUTOSCAN="autoscan${AUTOCONF_SUFFIX}"
export AUTOUPDATE="autoupdate${AUTOCONF_SUFFIX}"
export IFNAMES="ifnames${AUTOCONF_SUFFIX}"
export AUTOM4TE="autom4te${AUTOCONF_SUFFIX}"

export LIBTOOLIZE="libtoolize${LIBTOOL_SUFFIX}"
export LIBTOOL="libtool${LIBTOOL_SUFFIX}"

export AUTOGEN=autogen

(cd tests && $AUTOGEN ./Makefile.am.def)
(cd include && $AUTOGEN ./Makefile.am.def)
$AUTOGEN ./Makefile.am.def
$LIBTOOLIZE -vcif
$ACLOCAL -I m4 --install -Wall --force
$AUTOMAKE -vcaf
$AUTOCONF -I m4 -f -Wall
$AUTOM4TE --language=Autotest -I tests tests/testsuite.at -o tests/testsuite
