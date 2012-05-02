# flags
AM_CFLAGS = @PGLD_CFLAGS@
AM_CPPFLAGS = @PGLD_CPPFLAGS@
AM_LDFLAGS = @PGLD_LDFLAGS@

# sources for pgld binary
pgld_SOURCES = \
	       src/pgld.c src/pgld.h \
	       src/stream.c src/stream.h \
	       src/blocklist.c src/blocklist.h \
	       src/parser.c src/parser.h

sbin_PROGRAMS = pgld

# build dbus library for --enable-dbus
if DBUSMAKE
libdbusdir = $(libdir)/$(PACKAGE_NAME)
libdbus_LTLIBRARIES = libdbus.la
libdbus_la_SOURCES = src/dbus.c src/dbus.h
pgld_LDADD = \
	     @PGLD_LIBS@ \
	     ./libdbus.la

dbusdir = $(sysconfdir)/dbus-1/system.d
dbus_DATA = org.netfilter.pgl.conf
else
pgld_LDADD = \
	     @PGLD_LIBS@
endif

if LOGROTATEMAKE
logdir = $(sysconfdir)/logrotate.d
log_DATA = logrotate/pgld
endif

