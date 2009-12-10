project(pglcmd)
cmake_minimum_required(VERSION 2.6)

SET(GLOBALNAME pgl)
SET(PKGNAME pglcmd)
SET(PGLVERSION 2.0~pre1-1)

MESSAGE(STATUS "Configuring ${PKGNAME}")

INSTALL(PROGRAMS ${PKGNAME} DESTINATION bin)
INSTALL(PROGRAMS ${PKGNAME}.wd DESTINATION sbin)

INSTALL(FILES ${PKGNAME}.lib DESTINATION lib/${GLOBALNAME})
INSTALL(FILES ${PKGNAME}.main DESTINATION lib/${GLOBALNAME})
INSTALL(FILES ${PKGNAME}.defaults DESTINATION lib/${GLOBALNAME})

INSTALL(PROGRAMS logrotate DESTINATION /etc/logrotate.d RENAME ${PKGNAME})
INSTALL(PROGRAMS cron.daily DESTINATION /etc/cron.daily RENAME ${PKGNAME})
INSTALL(PROGRAMS if-up DESTINATION /etc/network/if-up.d RENAME ${PKGNAME})
INSTALL(PROGRAMS init DESTINATION /etc/init.d RENAME ${GLOBALNAME})

INSTALL(DIRECTORY DESTINATION /etc/${GLOBALNAME}/)
INSTALL(FILES allow.p2p DESTINATION /etc/${GLOBALNAME})
INSTALL(FILES blocklists.list DESTINATION /etc/${GLOBALNAME})
INSTALL(FILES ${PKGNAME}.conf DESTINATION /etc/${GLOBALNAME})

INSTALL(FILES .empty DESTINATION /var/lib/${GLOBALNAME} RENAME .empty_${PKGNAME})
INSTALL(FILES .empty DESTINATION /var/log/${GLOBALNAME} RENAME .empty_${PKGNAME})
INSTALL(FILES .empty DESTINATION /var/spool/${GLOBALNAME} RENAME .empty_${PKGNAME})

SET(CPACK_GENERATOR DEB)
SET(CPACK_SET_DESTDIR "ON")
SET(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
SET(CPACK_PACKAGE_NAME ${PKGNAME})
SET(CPACK_PACKAGE_VERSION ${PGLVERSION})
SET(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "iptables, lsb-base (>= 3.0-6), wget, unzip, p7zip | p7zip-full, pgld")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER peerguardian-devel@lists.sourceforge.net)
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "PeerGuardian Linux - ${PKGNAME}
 pgl blocks connections from and to hosts, that are listed in a very large
 blocklist. The blocklist can be in any of the following formats:
 eMule ipfilter.dat, peerguardian.p2p text, or peerguardian.p2b v2 binary.
 .
 This is the pgl command line interface ${PKGNAME}. It provides easy ways to
 interact with the pgl daemon and does all common related tasks.
 .
 Features:
  - Start and stop pgld. Or let init do this automatically.
  - Update your blocklists from online sources. Or let cron do this
    automatically. Backups will be used if a problem occurs. Additionally you
    may use your local blocklists.
  - Remove lines by keyword from the blocklists.
  - Handle your iptables rules: use a default setup, easily allow all traffic on
    specific ports and use an allow list, or add your own sophisticated iptables
    rules.
  - Automatically make sure the LAN and the DNS server aren't blocked. If you
    are on a public LAN, you probably want to disable this feature.
  - Check the status and test pgl.
  - Detects if kernel modules are needed and loads them if necessary.
  - Set verbosity and logging options.
  - Provides LSB 3.1 compatible init script.
  - Daily rotation of the logfiles.")
SET(CPACK_DEBIAN_PACKAGE_SECTION net)
SET(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE all)
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
INCLUDE(CPack)
