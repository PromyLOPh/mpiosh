#!/usr/bin/env bash

# autogen.sh 
#
# Andreas Buesching  <crunchy@tzi.de>
# $id$

# minimum needed software versions:
#
# * libtoolize (GNU libtool) 1.4.3
# * aclocal (GNU automake) 1.6.3
# * autoconf (GNU Autoconf) 2.57
# * automake (GNU automake) 1.6.3

export WANT_AUTOMAKE="1.7"

libtoolize -c -f && aclocal && autoconf && automake -a -c -f && ./configure $@

# end of autogen.sh 
