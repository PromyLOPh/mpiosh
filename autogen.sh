#!/usr/bin/env bash

# autogen.sh 
#
# Andreas Buesching  <crunchy@tzi.de>
# $ID$

aclocal && autoconf && automake -a && ./configure $@


# end of autogen.sh 
