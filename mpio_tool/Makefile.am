INCLUDES=-I..

bin_PROGRAMS=mpio_tool mpiosh

mpio_tool_SOURCES=mpio_tool.c
mpio_tool_LDADD=../libmpio/libmpio.la

mpiosh_SOURCES=mpiosh.c
mpiosh_LDADD=../libmpio/libmpio.la -lreadline -lncurses

noinst_HEADERS=mpiosh.h