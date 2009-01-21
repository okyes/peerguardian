#!/usr/bin/make -f
# Makefile (installation) for blockcontrol

clean:
	rm -f *~

install:
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/etc/blockcontrol
	install -d $(DESTDIR)/etc/default
	install -d $(DESTDIR)/usr/lib/blockcontrol
	install -d $(DESTDIR)/var/lib/blockcontrol
	install -d $(DESTDIR)/var/spool/blockcontrol
	install -m 755 blockcontrol $(DESTDIR)/usr/bin
	install -m 755 blockcontrol.defaults $(DESTDIR)/usr/lib/blockcontrol
	install -m 644 blockcontrol.lib $(DESTDIR)/usr/lib/blockcontrol
	install -m 644 allow.p2p blockcontrol.conf blocklists.list $(DESTDIR)/etc/blockcontrol
	install -m 755 iptables-custom-insert.sh iptables-custom-remove.sh $(DESTDIR)/etc/blockcontrol
	install -m 644 default $(DESTDIR)/etc/default/blockcontrol
	install -D -m 755 if-up $(DESTDIR)/etc/network/if-up.d/blockcontrol
	install -D -m 755 cron.daily $(DESTDIR)/etc/cron.daily/blockcontrol
	install -D -m 755 init $(DESTDIR)/etc/init.d/blockcontrol
# 	update-rc.d defaults 20
	install -D -m 644 logrotate $(DESTDIR)/etc/logrotate.d/blockcontrol

.PHONY: clean
