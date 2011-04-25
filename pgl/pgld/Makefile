#!/usr/bin/make -f
# Makefile for pgl component pgld
# This Makefile gets executed by the main pgl Makefile. Don't execute it directly!
# TODO: check this: ifeq (0,${MAKELEVEL})

CC=gcc
CFLAGS+= \
	-DVERSION=\"$(VERSION)\" \
	-DPACKAGE_NAME=\"$(PKGNAME)\" \
	-DPIDFILE=\"$(PIDDIR)/$(PKGNAME)d.pid\"

OBJS = \
	src/pgld.o \
	src/stream.o \
	src/blocklist.o \
	src/parser.o \
	src/dbus.o
LIBS = \
	-lnetfilter_queue \
	-lnfnetlink

ifeq ($(LOWMEM),yes)
DBUS=no
CFLAGS+=-DLOWMEM
endif

ifeq ($(ZLIB),yes)
CFLAGS+=-DHAVE_ZLIB
LIBS+=-lz
endif

ifeq ($(DBUS),yes)
CFLAGS+=-DHAVE_DBUS `pkg-config dbus-1 --cflags` -fPIC
LIBS+=-ldl
endif

ifeq ($(PROFILE),yes)
CFLAGS+=-pg
LDFLAGS+=-pg
endif

ifeq ($(DEBUG),yes)
CFLAGS+=-ggdb3
LDFLAGS+=-ggdb3
OPTFLAGS=-O0
endif

CFLAGS+=$(OPTFLAGS)

DISTDIR = ../$(PKGNAME)-$(VERSION)/pgld

DISTFILES = \
	Makefile \
	src/pgld.c src/pgld.h \
	src/blocklist.c src/blocklist.h \
	src/parser.c src/parser.h \
	src/stream.c src/stream.h \
	src/dbus.c src/dbus.h \
	dbus-pgld.conf logrotate

ifeq ($(DBUS),yes)
all: src/pgld src/dbus.so
else
all: src/pgld
endif

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

src/pgld: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

src/dbus.so: src/dbus.o
	$(CC) -shared -Wl `pkg-config dbus-1 --libs` -o $@ $^

clean:
	rm -rf *~ \
	src/*~ \
	src/*.o \
	src/pgld \
	src/dbus.so \
	$(DISTDIR)

install:
	install -D -m 755 src/pgld $(DESTDIR)/$(SBINDIR)/pgld
ifeq ($(DBUS),yes)
	install -D -m 644 dbus-pgld.conf $(DESTDIR)/$(DBUSCONFDIR)/pgld.conf
	install -D -m 644 src/dbus.so $(DESTDIR)/$(LIBDIR)/dbus.so
endif

install-strip: install
	strip $(DESTDIR)/$(SBINDIR)/pgld
ifeq ($(DBUS),yes)
	strip $(DESTDIR)/$(LIBDIR)/dbus.so
endif

dist:
	mkdir -p $(DISTDIR) $(DISTDIR)/src
	for I in $(DISTFILES) ; do cp "$$I" $(DISTDIR)/$$I ; done

.PHONY: all clean dist install install-strip
