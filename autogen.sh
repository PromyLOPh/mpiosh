#!/usr/bin/env bash

# autogen.sh 
#
# Andreas Buesching  <crunchy@tzi.de>
# $ID$

cd mplib
./autogen.sh
cd ..

aclocal && autoconf && automake -a && ./configure $@


# end of autogen.sh 
