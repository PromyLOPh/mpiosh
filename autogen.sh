#!/usr/bin/env bash

# autogen.sh 
#
# Andreas Buesching  <crunchy@tzi.de>
# $id$

aclocal && autoconf && automake -a && ./configure $@

# end of autogen.sh 
