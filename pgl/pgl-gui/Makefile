#!/usr/bin/make -f
# Makefile for pgl component pgl-gui
# This Makefile gets executed by the main pgl Makefile. Don't execute it directly!
# TODO: check this: ifeq (0,${MAKELEVEL})

CC=g++
MOC=moc-qt4
UIC=uic-qt4
RCC=rcc

# CFLAGS += \
# LDFLAGS += \

QTDIR:=/usr/share/qt4
QTINC:=$(QTDIR)/include
INCPATH:=\
	-I$(QTINC) \
	-I$(QTINC)/QtCore \
	-I$(QTINC)/QtNetwork \
	-I$(QTINC)/QtGui \
	-I. \
	-Isrc \
	-Iui

ifeq ($(LOWMEM),yes)
DBUS=no
# CFLAGS+=-DLOWMEM
endif

# from pgld Makefile:
# ifeq ($(DBUS),yes)
# CFLAGS+=-DHAVE_DBUS \
# 	`pkg-config dbus-1 --cflags --libs` \
# 	-fPIC
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

# source (.cpp files) with header (.h) file in folder src/
# so main.cpp is missing here because it has no header file
SOURCESDIR := src
SOURCES := $(addprefix $(SOURCESDIR)/, \
	add_exception_dialog.cpp \
	file_transactions.cpp \
	gui_options.cpp \
	ip_remove.cpp \
	ip_whitelist.cpp \
	list_add.cpp \
	moblock_settings.cpp \
	peerguardian.cpp \
	peerguardian_info.cpp \
	peerguardian_log.cpp \
	pglcmd.cpp \
	pgl_lists.cpp \
	pgl_settings.cpp \
	pgl_whitelist.cpp \
	proc_thread.cpp \
	settings.cpp \
	settings_manager.cpp \
	status_dialog.cpp \
	super_user.cpp \
	utils.cpp \
	whois.cpp \
	)

# forms (.ui files) in folder ui/
FORMSDIR := ui
FORMS := $(addprefix $(FORMSDIR)/, \
	add_exception.ui \
	ip_whitelist.ui \
	main_window.ui \
	status_dialog.ui \
	ip_remove.ui \
	list_add.ui \
	settings.ui \
	whois.ui \
	)

# prepare some filenames with "foreach" from the above variables
MOC_SOURCES := $(foreach SOURCE, $(SOURCES), $(SOURCESDIR)/moc_$(notdir $(SOURCE)))
MOC_SOURCES_OBJECTS = $(foreach SOURCE, $(SOURCES), $(SOURCESDIR)/moc_$(notdir $(SOURCE:cpp=o)))
FORMS_HEADERS := $(foreach FORM, $(FORMS), $(FORMSDIR)/ui_$(notdir $(FORM:ui=h)))


all: pgl-gui

# pgl-gui binary
# prequisites: objects of src/main.cpp, SOURCES, MOC_SOURCES and qrc_images
# original qmake recipe:
# g++
#   -o pgl-gui
#   build/obj/file_transactions.o build/obj/main.o build/obj/peerguardian_info.o build/obj/pgl_lists.o build/obj/peerguardian_log.o build/obj/pgl_settings.o build/obj/moblock_settings.o build/obj/peerguardian.o build/obj/super_user.o build/obj/pglcmd.o build/obj/settings.o build/obj/whois.o build/obj/list_add.o build/obj/settings_manager.o build/obj/status_dialog.o build/obj/ip_whitelist.o build/obj/ip_remove.o build/obj/proc_thread.o build/obj/utils.o build/obj/add_exception_dialog.o build/obj/pgl_whitelist.o build/obj/gui_options.o
#   build/obj/moc_pglcmd.o build/obj/moc_peerguardian_info.o build/obj/moc_peerguardian_log.o build/obj/moc_peerguardian.o build/obj/moc_super_user.o build/obj/moc_settings.o build/obj/moc_whois.o build/obj/moc_list_add.o build/obj/moc_settings_manager.o build/obj/moc_status_dialog.o build/obj/moc_ip_whitelist.o build/obj/moc_ip_remove.o build/obj/moc_proc_thread.o build/obj/moc_add_exception_dialog.o
#   build/obj/qrc_images.o
#   -L/usr/lib -lQtGui -lQtNetwork -lQtCore -lpthread
pgl-gui: $(SOURCESDIR)/main.o $(SOURCES:cpp=o) $(MOC_SOURCES_OBJECTS) qrc_images.o
	$(CC) \
		$(SOURCESDIR)/main.o \
		$(SOURCES:cpp=o) \
		$(MOC_SOURCES_OBJECTS) \
		qrc_images.o \
		-L/usr/lib -lQtGui -lQtNetwork -lQtCore -lpthread \
		-o $@

# objects of main and SOURCES
# target pattern: STEM.o
# STEM is e.g. "src/add_exception_dialog"
# prerequisite: STEM.cpp and perhaps FORMS_HEADERS
# example of original qmake recipe:
# g++
#  -c -pipe -g -D_REENTRANT -Wall -W -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED
#  -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4/QtNetwork -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I. -Isrc -Ibuild/moc -Iui
#  -o build/obj/add_exception_dialog.o
#  src/add_exception_dialog.cpp
$(SOURCESDIR)/main.o $(SOURCES:cpp=o): %.o: %.cpp $(FORMS_HEADERS)
	$(CC) \
		-pipe -g -D_REENTRANT -Wall -W -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED \
		$(INCPATH) \
		-c $< \
		-o $@

# moc source of SOURCES headers
# target-pattern: src/moc_STEM.cpp
# STEM is e.g. "add_exception_dialog"
# prerequisite is src/STEM.h
# example of original qmake recipe:
# /usr/bin/moc-qt4
#  -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED
#  -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4/QtNetwork -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I. -Isrc -Ibuild/moc -Iui
#  src/add_exception_dialog.h
#  -o build/moc/moc_add_exception_dialog.cpp
$(MOC_SOURCES): $(SOURCESDIR)/moc_%.cpp: $(SOURCESDIR)/%.h
	$(MOC) \
		-DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED \
		$(INCPATH) \
		$< \
		-o $@

# object of moc source of SOURCES headers
# target-pattern: src/moc_STEM.o
# STEM is e.g. "add_exception_dialog"
# prerequisite: src/moc_STEM.cpp and perhaps FORMS_HEADERS
# example of original qmake recipe:
# g++
#  -c -pipe -g -D_REENTRANT -Wall -W -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED
#  -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4/QtNetwork -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I. -Isrc -Ibuild/moc -Iui
#  -o build/obj/moc_add_exception_dialog.o
#  build/moc/moc_add_exception_dialog.cpp
$(MOC_SOURCES_OBJECTS): $(SOURCESDIR)/moc_%.o: $(SOURCESDIR)/moc_%.cpp $(FORMS_HEADERS)
	$(CC) \
		-pipe -g -D_REENTRANT -Wall -W -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED \
		$(INCPATH) \
		-c $< \
		-o $@

# headers of FORMS
# target pattern is ui/ui_STEM.h
# STEM is e.g. "add_exception"
# prerequisite is ui/STEM.ui
# example of original qmake recipe:
# /usr/bin/uic-qt4
#  ui/add_exception.ui
#  -o ui/ui_add_exception.h
$(FORMS_HEADERS): $(FORMSDIR)/ui_%.h: $(FORMSDIR)/%.ui
	$(UIC) \
		$< \
		-o $@

# original qmake recipe:
# g++
#  -c -pipe -g -D_REENTRANT -Wall -W -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED
#  -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4/QtNetwork -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I. -Isrc -Ibuild/moc -Iui
#  -o build/obj/qrc_images.o
#  qrc_images.cpp
qrc_images.o: qrc_images.cpp
	$(CC) \
		-pipe -g -D_REENTRANT -Wall -W -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED \
		$(INCPATH) \
		-c qrc_images.cpp \
		-o qrc_images.o

# original qmake recipe: /usr/bin/rcc -name images images/images.qrc -o qrc_images.cpp
qrc_images.cpp: images/images.qrc
	$(RCC) \
		-name images images/images.qrc -o qrc_images.cpp

###############################################################################

DISTDIR = ../$(PKGNAME)-$(VERSION)/pgl-gui

# These files are added to file releases.
# TODO: add e.g. new images
DISTFILES = \
	Makefile \
	src/main.cpp \
	$(SOURCES) \
	$(SOURCES:cpp=h) \
	$(FORMS) \
	images/backup.png \
	images/blocklist.png \
	images/cancel.png \
	images/LICENSE \
	images/editdelete.png \
	images/error.png \
	images/exit.png \
	images/forward.png \
	images/images.qrc \
	images/incoming.png \
	images/info.png \
	images/list_add.png \
	images/list_remove.png \
	images/log.png \
	images/ok.png \
	images/outgoing.png \
	images/pgl-gui.png \
	images/play.png \
	images/reload.png \
	images/restart.png \
	images/run.png \
	images/save.png \
	images/settings.png \
	images/stop.png \
	images/tray_disabled.png \
	images/tray.png \
	images/warning.png \
	other/pgl-gui.desktop

# Not distributed, because they are only there as last resort for the
# developers, in case this Makefile is broken:
# INSTALL
# pgl-gui.pro


install:
	install -D -m 755 pgl-gui $(DESTDIR)/$(BINDIR)/pgl-gui
	install -D -m 644 other/pgl-gui.desktop $(DESTDIR)/$(prefix)/share/applications/pgl-gui.desktop
	install -D -m 644 images/pgl-gui.png $(DESTDIR)/$(prefix)/share/pixmaps/pgl-gui.png

install-strip: install
	strip $(DESTDIR)/$(BINDIR)/pgl-gui

clean:
	rm -f *~ src/*~ \
	pgl-gui \
	$(SOURCESDIR)/*.o \
	$(MOC_SOURCES) \
	$(FORMS_HEADERS) \
	qrc_images.cpp \
	qrc_images.o

dist:
	mkdir -p $(DISTDIR) $(DISTDIR)/images $(DISTDIR)/other $(DISTDIR)/src $(DISTDIR)/ui
	for I in $(DISTFILES) ; do cp "$$I" $(DISTDIR)/$$I ; done

.PHONY: all install install-strip clean dist
