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
dist_log_DATA = logrotate/pglcmd
endif

if CRONMAKE
crondir = $(sysconfdir)/cron.daily
cron_SCRIPTS = cron.daily/pglcmd
endif

if NETWORKMANAGERMAKE
netdir = $(sysconfdir)/NetworkManager/dispatcher.d
net_SCRIPTS = networkmanager/20pglcmd
endif

initddir = @INITDDIR@
initd_SCRIPTS = init/pgl

pglcmddir = $(bindir)
pglcmd_SCRIPTS = pglcmd blockcontrol2pglcmd.sh

pglcmdwddir = $(sbindir)
pglcmdwd_SCRIPTS = pglcmd.wd

pgletcdir = $(sysconfdir)/$(PACKAGE_NAME)
dist_pgletc_DATA = \
	allow.p2p \
	blocklists.list \
	pglcmd.conf

pgllibdir = $(libdir)/$(PACKAGE_NAME)
pgllib_DATA = \
	pglcmd.defaults \
	pglcmd.lib \
	pglcmd.main

# Don't update PATH here anymore, because on user's make it doesn't contain
# [/usr]/sbin
pglcmd.defaults:
	sed \
	-e 's|^BINDIR=.*|BINDIR=\"$(bindir)\"|g' \
	-e 's|^SBINDIR=.*|SBINDIR=\"$(sbindir)\"|g' \
	-e 's|^PIDDIR=.*|PIDDIR=\"$(PIDDIR)\"|g' \
	-e 's|^LOGDIR=.*|LOGDIR=\"$(LOGDIR)\"|g' \
	-e 's|^CONFDIR=.*|CONFDIR=\"$(pgletcdir)\"|g' \
	-e 's|^LIBDIR=.*|LIBDIR=\"$(libdir)/$(PACKAGE_NAME)\"|g' \
	-e 's|^TMPDIR=.*|TMPDIR=\"$(TMPDIR)\"|g' \
	-e 's|^BLOCKLISTS_DIR=.*|BLOCKLISTS_DIR=\"$(BLOCKLISTS_DIR)\"|g' \
	-e 's|^LOCAL_BLOCKLIST_DIR=.*|LOCAL_BLOCKLIST_DIR=\"$(LOCAL_BLOCKLIST_DIR)\"|g' \
	-e 's|^MASTER_BLOCKLIST_DIR=.*|MASTER_BLOCKLIST_DIR=\"$(MASTER_BLOCKLIST_DIR)\"|g' \
	-e 's|^LSB=.*|LSB=\"$(LSB)\"|g' \
	-e 's|^DBUS=.*|DBUS=\"$(DBUS_SET)\"|g' \
	-e 's|^VERSION=.*|VERSION=\"$(VERSION)\"|g' \
	< pglcmd.defaults.in > pglcmd.defaults

pglcmd.main:
	sed \
	-e 's|^CMD_DEFAULTS=.*|CMD_DEFAULTS=\"$(pgllibdir)/pglcmd.defaults\"|g' \
	< pglcmd.main.in > pglcmd.main

do_subst = sed -e 's|^CONTROL_MAIN=.*|CONTROL_MAIN=\"$(pgllibdir)/pglcmd.main\"|g'

blockcontrol2pglcmd.sh:
	$(do_subst) < blockcontrol2pglcmd.sh.in > blockcontrol2pglcmd.sh
	chmod +x blockcontrol2pglcmd.sh

pglcmd:
	$(do_subst) < pglcmd.in > pglcmd
	chmod +x pglcmd

pglcmd.wd:
	$(do_subst) < pglcmd.wd.in > pglcmd.wd
	chmod +x pglcmd.wd

cron.daily/pglcmd:
	$(do_subst) < cron.daily/pglcmd.in > cron.daily/pglcmd
	chmod +x cron.daily/pglcmd

init/pgl:
	$(do_subst) < init/pgl.in > init/pgl
	chmod +x init/pgl

networkmanager/20pglcmd:
	$(do_subst) < networkmanager/20pglcmd.in > networkmanager/20pglcmd
	chmod +x networkmanager/20pglcmd

# These files are also distributed
# If the files get modified on "make", only the corresponding *.in files are
# listed here. Otherwise the file that gets installed is listed here.
EXTRA_DIST = \
	pglcmd.defaults.in \
	pglcmd.main.in \
	blockcontrol2pglcmd.sh.in \
	pglcmd.in \
	pglcmd.wd.in \
	cron.daily/pglcmd.in \
	init/pgl.in \
	networkmanager/20pglcmd.in \
	pglcmd.lib

# These files get removed on clean
CLEANFILES = \
	pglcmd.defaults \
	pglcmd.main \
	blockcontrol2pglcmd.sh \
	pglcmd \
	pglcmd.wd \
	cron.daily/pglcmd \
	init/pgl \
	networkmanager/20pglcmd
