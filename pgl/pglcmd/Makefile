#!/usr/bin/make -f
# Makefile (installation) for pglcmd

SHELL = /bin/sh

clean:
	rm -f *~

install:
	install -D -m 755 pglcmd $(DESTDIR)/usr/bin/pglcmd
	install -D -m 755 pglcmd.main $(DESTDIR)/usr/lib/pgl/pglcmd.main
	install -D -m 644 pglcmd.defaults $(DESTDIR)/usr/lib/pgl/pglcmd.defaults
	install -D -m 644 pglcmd.lib $(DESTDIR)/usr/lib/pgl/pglcmd.lib
	install -D -m 755 pglcmd.wd $(DESTDIR)/usr/bin/pglcmd.wd
	install -D -m 644 allow.p2p $(DESTDIR)/etc/pgl/allow.p2p
	install -D -m 644 pglcmd.conf $(DESTDIR)/etc/pgl/pglcmd.conf
	install -D -m 644 blocklists.list $(DESTDIR)/etc/pgl/blocklists.list
	install -D -m 755 if-up $(DESTDIR)/etc/network/if-up.d/pglcmd
	install -D -m 755 cron.daily $(DESTDIR)/etc/cron.daily/pglcmd
	install -D -m 755 init $(DESTDIR)/etc/init.d/pglcmd
# 	update-rc.d pglcmd start 60 2 3 4 5 . stop 20 0 1 6 .
	install -D -m 644 logrotate $(DESTDIR)/etc/logrotate.d/pglcmd
	install -d $(DESTDIR)/var/lib/pgl
	install -d $(DESTDIR)/var/spool/pgl

.PHONY: clean
