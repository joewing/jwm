#!/bin/sh
automake -a
autoreconf --install --force
touch config.rpath
