INCLUDES=-I/usr/src/linux/include
KCFLAGS=-D__KERNEL__ -I$(INCLUDE) -Wall -Wstrict-prototypes -O2 \
	-fomit-frame-pointer -pipe -DMODULE

EXTRA_DIST=mpio.c

all: mpio.o

mpio.o: mpio.c
	$(CC) -c $(KCFLAGS) $(INCLUDES) $<

clean:
	rm -f mpio.o

distclean:
	rm -f mpio.o

install-data-local:
	$(mkinstalldirs) $(DESTDIR)@MODULE_PATH@
	$(INSTALL_SCRIPT) -m 644 mpio.o $(DESTDIR)@MODULE_PATH@
