cmake_minimum_required (VERSION 2.4)

set (PACKAGE mpiosh)
project (${PACKAGE} C)

set (CMAKE_C_FLAGS -Wall)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable (mpiosh mpiosh.c callback.c readline.c command.c global.c
	cfgio.c cfg.c)
target_link_libraries (mpiosh mpio -lreadline -lncurses)
