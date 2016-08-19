#!/bin/sh
automake -a -c
autoreconf --install --force
touch config.rpath
