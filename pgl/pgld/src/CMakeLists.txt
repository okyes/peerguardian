MESSAGE(STATUS "Configuring pgld")

SET(DAEMONNAME pgld)
SET(VERSION 2.0~pre1-1)
SET(SOURCES blocklist.c parser.c pgld.c stream.c)

INCLUDE(FindPkgConfig)

ADD_DEFINITIONS(-Wall -O2 -DPACKAGE_NAME="${DAEMONNAME}" -DVERSION="${VERSION}" -DPIDFILE="/var/run/${DAEMONNAME}.pid")

MESSAGE(STATUS "Looking for libnetfilter_queue...")
pkg_check_modules(NFQUEUE REQUIRED libnetfilter_queue)


MESSAGE(STATUS "Looking for libnfnetlink...")
pkg_check_modules(NFLINK REQUIRED libnfnetlink)

if(NOT NODBUS AND NOT LOWMEM)
    MESSAGE(STATUS "Looking for dbus-1...")
    pkg_check_modules(DBUS dbus-1)
endif(NOT NODBUS AND NOT LOWMEM)


ADD_EXECUTABLE(pgld ${SOURCES})
TARGET_LINK_LIBRARIES(pgld ${NFQUEUE_LIBRARIES} ${NFLINK_LIBRARIES})
if (LOWMEM)
    ADD_DEFINITIONS(-DLOWMEM)
elseif (DBUS_FOUND)
    ADD_DEFINITIONS(-DHAVE_DBUS)
    TARGET_LINK_LIBRARIES(pgld ${DBUS_LIBRARIES})
endif(LOWMEM)

INSTALL(TARGETS pgld RUNTIME DESTINATION sbin)
