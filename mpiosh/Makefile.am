INCLUDES=-I..

bin_PROGRAMS=mpiosh

mpiosh_SOURCES = mpiosh.c \
		callback.c \
		readline.c \
		command.c \
		global.c \
		cfgio.c \
		config.c

AM_CFLAGS=-DSYSCONFDIR=\"@sysconfdir@\"

mpiosh_LDADD=../libmpio/libmpio.la -lreadline -lncurses

noinst_HEADERS = mpiosh.h \
		callback.h \
		readline.h \
		command.h \
		global.h \
		cfgio.h \
		config.h