#!/usr/bin/env bash

# autogen.sh 
#
# Andreas Buesching  <crunchy@tzi.de>
# $id$

export WANT_AUTOMAKE="1.7"

aclocal && autoconf && automake -a && ./configure $@

# end of autogen.sh 
