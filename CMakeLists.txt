cmake_minimum_required (VERSION 2.4)

set (PACKAGE mpiosh)
project (${PACKAGE} C)

find_library (LIBNCURSES NAMES ncurses)
find_library (LIBREADLINE NAMES readline)

if (LIBNCURSES)
	message (STATUS "found ncurses at ${LIBNCURSES}")
else (LIBNCURSES)
	message (FATAL_ERROR "ncurses is required")
endif (LIBNCURSES)

if (LIBREADLINE)
	message (STATUS "found readline at ${LIBREADLINE}")
else (LIBREADLINE)
	message (FATAL_ERROR "readline is required")
endif (LIBREADLINE)

add_subdirectory (libmpio)
add_subdirectory (src)
add_subdirectory (tools)
