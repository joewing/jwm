#!/bin/sh
mkdir -p m4
aclocal -I m4
automake -ac
autoreconf -fi -I m4
touch config.rpath
