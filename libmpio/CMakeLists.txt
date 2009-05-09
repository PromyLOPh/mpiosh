cmake_minimum_required (VERSION 2.4)

set (PACKAGE libmpio)
project (${PACKAGE} C)

set (CMAKE_C_FLAGS -Wall)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/mplib)

add_library (mpio STATIC src/mpio.c src/io.c src/debug.c src/smartmedia.c
	src/mmc.c src/directory.c src/fat.c src/ecc.c src/cis.c src/id3.c
	mplib/mplib.c mplib/mplib_paas.c mplib/mplib_s.c mplib/xmalloc.c)

target_link_libraries (mpio -lusb)
