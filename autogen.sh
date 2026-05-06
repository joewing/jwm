#!/bin/sh

MACOS_M4_DIR=/opt/homebrew/share/gettext/m4

aclocal
automake -ac

if [ -e $MACOS_M4_DIR ] ; then
   autoreconf -fi -I $MACOS_M4_DIR
else
   autoreconf -fi
fi

touch config.rpath
