INCLUDES=@MPLIB_INCLUDE@
AM_CFLAGS=@MPLIB_CFLAGS@ 

lib_LTLIBRARIES=libmpio.la 

libmpio_la_SOURCES= \
		src/mpio.c \
		src/io.c \
		src/debug.c \
		src/smartmedia.c \
		src/directory.c \
		src/fat.c \
		src/ecc.c \
		src/cis.c \
		src/id3.c \
		mplib/mplib.c \
		mplib/mplib_paas.c \
		mplib/mplib_s.c \
		mplib/xmalloc.c

pkginclude_HEADERS= \
		src/mpio.h \
		src/defs.h

noinst_HEADERS= \
		src/io.h \
		src/debug.h \
		src/smartmedia.h \
		src/directory.h \
		src/fat.h \
		src/ecc.h \
		src/cis.h \
		src/id3.h \
		mplib/mplib.h \
		mplib/mplib_s.h \
		mplib/xmalloc.h

