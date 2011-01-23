#!/usr/bin/make -f
# Makefile for pgl

#
# General settings
#
PKGNAME = pgl
VERSION = git
SHELL = /bin/sh

# Compilation settings
CFLAGS=-Wall
OPTFLAGS=-Os

#
# Set which pgl modules shall be built
#
MAKE_PGLD ?= yes
MAKE_PGLCMD ?= yes
# NOTE: currently pgl-gui compiles, but is not usable yet.
MAKE_PGLGUI ?= no

#
# Configuration settings
#

# Set DBUS to yes if you want to be able to use DBUS.
DBUS ?= no

# Set ZLIB to yes if you want pgld to be able to load compressed blocklists.
ZLIB ?= yes

# LOWMEM disables storing of textual range descriptions in RAM.
# Set to yes if you are building a version for embedded devices
# like router or NAS box.
LOWMEM ?= no

# Want to run gprof?
PROFILE ?= no

# Want to use gdb on the target binary?
DEBUG ?= yes


#
# Path settings
#
# Note: the pgld/logrotate and pglcmd/logrotate files need to be adapted
# manually if you change things here.
prefix ?= /usr
# PATH is already set, so use custom PATHS as variable name.
PATHS ?= /usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
BINDIR ?= $(prefix)/bin
SBINDIR ?= $(prefix)/sbin
PIDDIR ?= /var/run
LOGDIR ?= /var/log/pgl
CONFDIR ?= /etc/pgl
LIBDIR ?= $(prefix)/lib/pgl
TMPDIR ?= /tmp
BLOCKLISTS_DIR ?= /var/spool/pgl
MASTER_BLOCKLIST_DIR ?= /var/lib/pgl
DBUSCONFDIR ?= /etc/dbus-1/system.d
IFUPDIR ?= /etc/network/if-up.d
CRONDIR ?= /etc/cron.daily
INITDIR ?= /etc/init.d
LOGROTATEDIR ?= /etc/logrotate.d
# If LSB file does not exist or variable is set to an empty value, pglcmd's
# builtin LSB functions are used.
LSB ?= /lib/lsb/init-functions


# Evaluate which components are to be built
ifeq ($(MAKE_PGLD),yes)
MODULES_ALL+=pgld_all
MODULES_INSTALL+=pgld_install
MODULES_INSTALLSTRIP+=pgld_install-strip
MODULES_CLEAN+=pgld_clean
MODULES_DIST+=pgld_dist
endif
ifeq ($(MAKE_PGLCMD),yes)
MODULES_ALL+=pglcmd_all
MODULES_INSTALL+=pglcmd_install
MODULES_CLEAN+=pglcmd_clean
MODULES_DIST+=pglcmd_dist
endif
ifeq ($(MAKE_PGLGUI),yes)
MODULES_ALL+=pgl-gui_all
MODULES_INSTALL+=pgl-gui_install
MODULES_INSTALLSTRIP+=pgl-gui_install-strip
MODULES_CLEAN+=pgl-gui_clean
MODULES_DIST+=pgl-gui_dist
RELEASEFILE:=$(PKGNAME)-$(VERSION).tar.gz
else
RELEASEFILE:=$(PKGNAME)_no-GUI-$(VERSION).tar.gz
endif

DISTDIR = $(PKGNAME)-$(VERSION)

DISTFILES = \
	debian/changelog \
	debian/compat \
	debian/control \
	debian/copyright \
	debian/pglcmd.config \
	debian/pglcmd.dirs \
	debian/pglcmd.examples \
	debian/pglcmd.install \
	debian/pglcmd.links \
	debian/pglcmd.postinst \
	debian/pglcmd.postrm \
	debian/pglcmd.preinst \
	debian/pglcmd.templates \
	debian/pgld.dirs \
	debian/pgld.docs \
	debian/pgld.install \
	debian/pgld.manpages \
	debian/pgl-gui.install \
	debian/pgl-gui.links \
	debian/po/POTFILES.in \
	debian/po/templates.pot \
	debian/README.Debian \
	debian/rules \
	debian/source/format \
	debian/watch \
	docs/AUTHORS \
	docs/BUGS \
	docs/COPYING \
	docs/iptables-custom-insert.sh \
	docs/iptables-custom-remove.sh \
	docs/pgld.1 \
	docs/README \
	docs/README.blocklists \
	docs/THANKS \
	docs/TODO \
	ChangeLog \
	INSTALL \
	Makefile

all: $(MODULES_ALL)

install: $(MODULES_INSTALL)

install-strip: $(MODULES_INSTALLSTRIP)

clean: $(MODULES_CLEAN)
	rm -rf *~ debian/*~ docs/*~ $(DISTDIR)

dist: $(MODULES_DIST)
	mkdir -p $(DISTDIR) $(DISTDIR)/debian $(DISTDIR)/debian/po $(DISTDIR)/debian/source $(DISTDIR)/docs
	for I in $(DISTFILES) ; do cp "$$I" $(DISTDIR)/$$I ; done
	chmod +x debian/rules
	tar cvzf $(RELEASEFILE) $(DISTDIR)
	rm -rf $(DISTDIR)

$(MODULES_ALL):
	$(MAKE) -C $(subst _all,,$@) all

$(MODULES_INSTALL):
	$(MAKE) -C $(subst _install,,$@) install

$(MODULES_INSTALLSTRIP):
	$(MAKE) -C $(subst _install-strip,,$@) install-strip

$(MODULES_CLEAN):
	$(MAKE) -C $(subst _clean,,$@) clean

$(MODULES_DIST):
	$(MAKE) -C $(subst _dist,,$@) dist

# Simply by being mentioned as a target, this tells make to export all variables to child processes by default.
.EXPORT_ALL_VARIABLES:

.PHONY: all $(MODULES_ALL) \
	install $(MODULES_INSTALL) \
	install-strip $(MODULES_INSTALLSTRIP) \
	clean $(MODULES_CLEAN) \
	dist $(MODULES_DIST)
