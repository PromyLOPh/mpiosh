INCLUDES=-I/usr/src/linux/include
KCFLAGS=-D__KERNEL__ -I$(INCLUDE) -Wall -Wstrict-prototypes -O2 \
	-fomit-frame-pointer -pipe -DMODULE

bin_PROGRAMS=mpio.o

mpio_o_SOURCES=mpio.c

all: mpio.o

mpio.o: mpio.c
	$(CC) -c $(KCFLAGS) $(INCLUDES) $<

