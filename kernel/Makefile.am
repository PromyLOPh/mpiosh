INCLUDES=-I/usr/src/linux/include
KCFLAGS=-D__KERNEL__ -I$(INCLUDE) -Wall -Wstrict-prototypes -O2 \
	-fomit-frame-pointer -pipe -DMODULE

EXTRA_DIST=mpio.c

all: mpio.o

mpio.o: mpio.c
	$(CC) -c $(KCFLAGS) $(INCLUDES) $<

install-data-local:
	$(mkinstalldirs) $(DESTDIR)@MODULE_PATH@
	$(INSTALL_PROGRAM) -m 644 mpio.o $(DESTDIR)@MODULE_PATH@

# install-strip:
# 	$(mkinstalldirs) $(DESTDIR)@MODULE_PATH@
# 	$(INSTALL_PROGRAM) -m 644 mpio.o $(DESTDIR)@MODULE_PATH@

# install:
# 	$(mkinstalldirs) $(DESTDIR)@MODULE_PATH@
# 	$(INSTALL_PROGRAM) -m 644 mpio.o $(DESTDIR)@MODULE_PATH@
