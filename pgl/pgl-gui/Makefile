#!/usr/bin/make -f
# Makefile for pgl component pgl-gui
# This Makefile gets executed by the main pgl Makefile. Don't execute it directly!
# TODO: check this: ifeq (0,${MAKELEVEL})

CC=g++
CFLAGS=-Wall
OBJS = \
	main.o \
	abstracthandler.o \
	debug.o \
	filehandler.o \
	loghandler.o \
	processhandler.o
DISTDIR = ../$(PKGNAME)-$(VERSION)/pgl-gui
DISTFILES = \
	COPYING \
	Makefile \
	regexps.txt \
	src/abstracthandler.cpp src/abstracthandler.h \
	src/debug.cpp src/debug.h \
	src/filehandler.cpp src/filehandler.h \
	src/loghandler.cpp src/loghandler.h \
	src/main.cpp \
	src/processhandler.cpp src/processhandler.h
# FIXME: Qt stuff is not found (but compiling with cmake works).
QTDIR=/usr/share/qt4/
QTINC=$(QTDIR)/include/
QTLIB=$(QTDIR)/lib/


all: src/pgl-gui

src/pgl-gui: $(OBJS)
	$(CC) $(OBJS) -o src/pgl-gui

$(OBJS): %.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

install:
	install -D -m 755 src/pgl-gui $(DESTDIR)/$(BINDIR)/pgl-gui

install-strip: install
	strip $(DESTDIR)/$(BINDIR)/pgl-gui

clean:
	rm -f *~ src/*~ $(OBJS) src/pgl-gui

dist:
	mkdir -p $(DISTDIR) $(DISTDIR)/src
	for I in $(DISTFILES) ; do cp "$$I" $(DISTDIR)/$$I ; done

.PHONY: all clean dist install install-strip
