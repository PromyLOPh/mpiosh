INCLUDES=-I@KERNEL_INCLUDE@
KCFLAGS=-D__KERNEL__ -Wall -Wstrict-prototypes -O2 \
	-fomit-frame-pointer -pipe -DMODULE

EXTRA_DIST=mpio.c

all: @MPIO_KERNEL@

kernel: mpio.o

mpio.o: mpio.c
	$(CC) -c $(KCFLAGS) $(INCLUDES) $<

clean:
	rm -f mpio.o

distclean:
	rm -f mpio.o

install-data-local:
	@test -n "@MODULE_PATH@" || exit 0
	$(mkinstalldirs) $(DESTDIR)@MODULE_PATH@
	$(INSTALL_SCRIPT) -m 644 mpio.o $(DESTDIR)@MODULE_PATH@
