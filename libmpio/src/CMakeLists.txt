set (CMAKE_C_FLAGS -Wall)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../)

add_library (mpio STATIC mpio.c io.c debug.c smartmedia.c mmc.c directory.c
	fat.c ecc.c cis.c)

target_link_libraries (mpio -lusb)

