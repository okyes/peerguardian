project(pgld)
cmake_minimum_required(VERSION 2.6)

SET(PKGNAME pgld)

add_subdirectory(src)


INSTALL(FILES logrotate DESTINATION /etc/logrotate.d RENAME ${PKGNAME})

SET(CPACK_GENERATOR DEB)
SET(CPACK_SET_DESTDIR "ON")
SET(CPACK_PACKAGE_NAME ${PKGNAME})
SET(CPACK_PACKAGE_VERSION ${PGLVERSION})
SET(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libnetfilter-queue1 (>= 0.0.17-1), libnfnetlink0 (>=1.0.0-1)")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER peerguardian-devel@lists.sourceforge.net)
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "PeerGuardian Linux - ${PKGNAME}
 pgl blocks connections from and to hosts, that are listed in a very large
 blocklist. The blocklist can be in any of the following formats:
 eMule ipfilter.dat, peerguardian.p2p text, and peerguardian.p2b binary.
 .
 This is the pgl daemon ${PKGNAME}. It uses the netfilter xt_NFQUEUE kernel module
 (available since kernel 2.6.13).")
SET(CPACK_DEBIAN_PACKAGE_SECTION net)
SET(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
IF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    FIND_PROGRAM(DPKG_CMD dpkg)
    IF(NOT DPKG_CMD)
        MESSAGE(STATUS "Can not find dpkg in your path, default to i386.")
        SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
    ENDIF(NOT DPKG_CMD)
    EXECUTE_PROCESS(COMMAND "${DPKG_CMD}" --print-architecture
        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ENDIF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
INCLUDE(CPack)


