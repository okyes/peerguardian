project(pglcmd)
cmake_minimum_required(VERSION 2.6)

MESSAGE(STATUS "Configuring pglcmd")

INSTALL(PROGRAMS pglcmd DESTINATION bin)
INSTALL(PROGRAMS pglcmd.wd DESTINATION sbin)

INSTALL(FILES pglcmd.lib DESTINATION lib/pgl)
INSTALL(FILES pglcmd.main DESTINATION lib/pgl)
INSTALL(FILES pglcmd.defaults DESTINATION lib/pgl)

INSTALL(FILES logrotate DESTINATION /etc/logrotate.d RENAME pglcmd)
INSTALL(FILES cron.daily DESTINATION /etc/cron.daily RENAME pglcmd)
INSTALL(FILES if-up DESTINATION /etc/network/if-up.d RENAME pglcmd)
INSTALL(FILES init DESTINATION /etc/init.d RENAME pgl)

INSTALL(CODE "EXECUTE_PROCESS(COMMAND 'test -d /etc/pgl && mkdir -p /etc/pgl')")
INSTALL(FILES allow.p2p DESTINATION /etc/pgl)
INSTALL(FILES blocklists.list DESTINATION /etc/pgl)
INSTALL(FILES pglcmd.conf DESTINATION /etc/pgl)

# INSTALL(CODE "EXECUTE_PROCESS(COMMAND 'test -d /var/lib/pgl && mkdir -p /var/lib/pgl')")