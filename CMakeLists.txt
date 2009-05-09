cmake_minimum_required (VERSION 2.4)

set (PACKAGE mpiosh)
project (${PACKAGE} C)

add_subdirectory (libmpio)
add_subdirectory (src)
