#!/usr/bin/make -f
# Makefile for pgl component pglcmd
# This Makefile gets executed by the main pgl Makefile. Don't execute it directly!
# TODO: check this: ifeq (0,${MAKELEVEL})

DISTDIR = ../$(PKGNAME)-$(VERSION)/pglcmd

DISTFILES = \
	allow.p2p \
	blocklists.list \
	cron.daily \
	if-up \
	init \
	logrotate \
	Makefile \
	pglcmd \
	pglcmd.conf \
	pglcmd.defaults \
	pglcmd.lib \
	pglcmd.main \
	pglcmd.wd

all:
	cat pglcmd.defaults | \
	sed "s|^PATH=.*|PATH=\"$(PATHS)\"|"           | \
	sed "s|^BINDIR=.*|BINDIR=\"$(BINDIR)\"|"     | \
	sed "s|^SBINDIR=.*|SBINDIR=\"$(SBINDIR)\"|"  | \
	sed "s|^PIDDIR=.*|PIDDIR=\"$(PIDDIR)\"|"     | \
	sed "s|^LOGDIR=.*|LOGDIR=\"$(LOGDIR)\"|"     | \
	sed "s|^CONFDIR=.*|CONFDIR=\"$(CONFDIR)\"|"  | \
	sed "s|^LIBDIR=.*|LIBDIR=\"$(LIBDIR)\"|"     | \
	sed "s|^BLOCKLISTS_DIR=.*|BLOCKLISTS_DIR=\"$(BLOCKLISTS_DIR)\"|"                   | \
	sed "s|^MASTER_BLOCKLIST_DIR=.*|MASTER_BLOCKLIST_DIR=\"$(MASTER_BLOCKLIST_DIR)\"|" | \
	sed "s|^LSB=.*|LSB=\"$(LSB)\"|" > pglcmd.defaults.install

	cat pglcmd.main | \
	sed "s|^CMD_DEFAULTS=.*|CMD_DEFAULTS=\"$(LIBDIR)/pglcmd.defaults\"|" > pglcmd.main.install

	for FILE in pglcmd pglcmd.wd cron.daily init ../debian/postinst if-up ; do \
		cat $$FILE | \
		sed "s|^CONTROL_MAIN=.*|CONTROL_MAIN=\"$(LIBDIR)/pglcmd.main\"|" > $$FILE.install ; \
	done
	# Not clean because the original file is changed here. The debian/postinst should be handled by the debian/rules file.
	# Just doing this here to avoid to forget this. Normally this file shouldn't change anyway.

install:
	install -D -m 755 pglcmd.install $(DESTDIR)/$(BINDIR)/pglcmd
	install -D -m 644 pglcmd.main.install $(DESTDIR)/$(LIBDIR)/pglcmd.main
	install -D -m 644 pglcmd.defaults.install $(DESTDIR)/$(LIBDIR)/pglcmd.defaults
	install -D -m 644 pglcmd.lib $(DESTDIR)/$(LIBDIR)/pglcmd.lib
	install -D -m 755 pglcmd.wd.install $(DESTDIR)/$(SBINDIR)/pglcmd.wd
	install -D -m 644 allow.p2p $(DESTDIR)/$(CONFDIR)/allow.p2p
	install -D -m 644 pglcmd.conf $(DESTDIR)/$(CONFDIR)/pglcmd.conf
	install -D -m 644 blocklists.list $(DESTDIR)/$(CONFDIR)/blocklists.list
	install -D -m 755 if-up.install $(DESTDIR)/$(IFUPDIR)/pglcmd
	install -D -m 755 cron.daily.install $(DESTDIR)/$(CRONDIR)/pglcmd
	install -D -m 755 init.install $(DESTDIR)/$(INITDIR)/pgl
# 	Uncomment if you want LSB 3.2 init script activation
# 	/usr/lib/lsb/install_initd /etc/init.d/pgl
# 	Uncomment if you want update-rc.d init script activation (like in Debian)
# 	update-rc.d pgl start 60 2 3 4 5 . stop 20 0 1 6 .
	install -D -m 644 logrotate $(DESTDIR)/$(LOGROTATEDIR)/pglcmd
	install -d $(DESTDIR)/$(MASTER_BLOCKLIST_DIR)
	install -d $(DESTDIR)/$(BLOCKLISTS_DIR)

clean:
	rm -f *~ pglcmd.defaults.install pglcmd.main.install pglcmd.install pglcmd.wd.install cron.daily.install init.install ../debian/postinst.install if-up.install

dist:
	mkdir -p $(DISTDIR)
	for I in $(DISTFILES) ; do cp "$$I" $(DISTDIR)/$$I ; done
	chmod +x cron.daily if-up init pglcmd pglcmd.wd

.PHONY: all clean dist install
