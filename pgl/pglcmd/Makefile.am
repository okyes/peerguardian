PIDDIR = @PIDDIR@
LOGDIR = @LOGDIR@
TMPDIR = @TMPDIR@
BLOCKLISTS_DIR = @BLOCKLISTS_DIR@
LOCAL_BLOCKLIST_DIR = @LOCAL_BLOCKLIST_DIR@
MASTER_BLOCKLIST_DIR = @MASTER_BLOCKLIST_DIR@
LSB = @LSBFILE@

if DBUSMAKE
DBUS_SET = 1
else
DBUS_SET = 0
endif

if LOGROTATEMAKE
logdir = $(sysconfdir)/logrotate.d
log_DATA = logrotate/pglcmd
endif

if CRONMAKE
crondir = $(sysconfdir)/cron.daily
cron_SCRIPTS = cron.daily/pglcmd
endif

if NETWORKMANAGERMAKE
netdir = $(sysconfdir)/NetworkManager/dispatcher.d
net_SCRIPTS = networkmanager/20pglcmd
endif

initddir = @INITDIR@
initd_SCRIPTS = init/pgl

pglcmddir = $(bindir)
pglcmd_SCRIPTS = pglcmd blockcontrol2pglcmd.sh

pglcmdwddir = $(sbindir)
pglcmdwd_SCRIPTS = pglcmd.wd

pgletcdir = $(sysconfdir)/$(PACKAGE_NAME)
pgletc_DATA = \
	allow.p2p \
	blocklists.list \
	pglcmd.conf

pgllibdir = $(libdir)/$(PACKAGE_NAME)
pgllib_DATA = \
	pglcmd.defaults \
	pglcmd.lib \
	pglcmd.main

all-local:
	cat pglcmd.defaults | \
	$(SED) "s|^PATH=.*|PATH=\"$(PATH)\"|" | \
	$(SED) "s|^BINDIR=.*|BINDIR=\"$(bindir)\"|" | \
	$(SED) "s|^SBINDIR=.*|SBINDIR=\"$(sbindir)\"|" | \
	$(SED) "s|^PIDDIR=.*|PIDDIR=\"$(PIDDIR)\"|" | \
	$(SED) "s|^LOGDIR=.*|LOGDIR=\"$(LOGDIR)\"|" | \
	$(SED) "s|^CONFDIR=.*|CONFDIR=\"$(pgletcdir)\"|" | \
	$(SED) "s|^LIBDIR=.*|LIBDIR=\"$(libdir)/$(PACKAGE_NAME)\"|" | \
	$(SED) "s|^TMPDIR=.*|TMPDIR=\"$(TMPDIR)\"|" | \
	$(SED) "s|^BLOCKLISTS_DIR=.*|BLOCKLISTS_DIR=\"$(BLOCKLISTS_DIR)\"|" | \
	$(SED) "s|^LOCAL_BLOCKLIST_DIR=.*|LOCAL_BLOCKLIST_DIR=\"$(LOCAL_BLOCKLIST_DIR)\"|" | \
	$(SED) "s|^MASTER_BLOCKLIST_DIR=.*|MASTER_BLOCKLIST_DIR=\"$(MASTER_BLOCKLIST_DIR)\"|" | \
	$(SED) "s|^LSB=.*|LSB=\"$(LSB)\"|" | \
	$(SED) "s|^DBUS=.*|DBUS=\"$(DBUS_SET)\"|" > pglcmd.defaults.install
	mv -f pglcmd.defaults.install pglcmd.defaults
	$(SED) -i -e "s|^CMD_DEFAULTS=.*|CMD_DEFAULTS=\"$(pgllibdir)/pglcmd.defaults\"|" pglcmd.main
	for FILE in blockcontrol2pglcmd.sh pglcmd pglcmd.wd cron.daily/pglcmd init/pgl ../debian/pglcmd.postinst networkmanager/20pglcmd ; do \
		cat $$FILE | \
		$(SED) -i -e "s|^CONTROL_MAIN=.*|CONTROL_MAIN=\"$(pgllibdir)/pglcmd.main\"|" $$FILE ; \
	done
