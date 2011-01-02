#!/usr/bin/make -f
# Makefile for pgl component pgl-gui
# This Makefile gets executed by the main pgl Makefile. Don't execute it directly!
# TODO: check this: ifeq (0,${MAKELEVEL})


QTDIR=/usr/share/qt4
QTINC=$(QTDIR)/include

CC=g++

ifeq ($(LOWMEM),yes)
DBUS=no
# CFLAGS+=-DLOWMEM
endif

# from pgld Makefile:
# ifeq ($(DBUS),yes)
# CFLAGS+=-DHAVE_DBUS `pkg-config dbus-1 --cflags` -fPIC
# LIBS+=-ldl
# endif

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






OBJS = \
	src/main.o \
	src/abstracthandler.o \
	src/debug.o \
	src/filehandler.o \
	src/loghandler.o \
	src/processhandler.o
DISTDIR = ../$(PKGNAME)-$(VERSION)/pgl-gui
DISTFILES = \
	COPYING \
	Makefile \
	regexps.txt \
	src/main.cpp \
	$(OBJS:o=cpp) \
	$(OBJS:o=h)

all: src/pgl-gui

src/pgl-gui: $(OBJS)
	moc -f $< -o $(@:o=moc)
	$(CC) $(LDFLAGS) -I$(QTINC) -I$(QTINC)/QtCore -I. -I./src -lQtCore $(OBJS) -o $@

$(OBJS): %.o: %.cpp
	moc -f $(<:cpp=h) -o $(@:o=moc)
	$(CC) $(CFLAGS) -I$(QTINC) -I$(QTINC)/QtCore -c $< -o $@

install:
	install -D -m 755 src/pgl-gui $(DESTDIR)/$(BINDIR)/pgl-gui

install-strip: install
	strip $(DESTDIR)/$(BINDIR)/pgl-gui

clean:
	rm -f *~ src/*~ \
	$(OBJS) \
	$(OBJS:o=moc) \
	src/pgl-gui \

dist:
	mkdir -p $(DISTDIR) $(DISTDIR)/src
	for I in $(DISTFILES) ; do cp "$$I" $(DISTDIR)/$$I ; done

.PHONY: all clean dist install install-strip
