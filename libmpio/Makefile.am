INCLUDES=@MPLIB_INCLUDE@
AM_CFLAGS=@MPLIB_CFLAGS@ 

lib_LTLIBRARIES=libmpio.la 
libmpio_la_LIBADD=@MPLIB_LIBADD@

libmpio_la_SOURCES= \
		mpio.c \
		io.c \
		debug.c \
		smartmedia.c \
		directory.c \
		fat.c \
		ecc.c \
		cis.c \
		id3.c

pkginclude_HEADERS= \
		mpio.h \
		defs.h

noinst_HEADERS= \
		io.h \
		debug.h \
		smartmedia.h \
		directory.h \
		fat.h \
		ecc.h \
		cis.h \
		id3.h
