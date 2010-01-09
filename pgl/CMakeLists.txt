project(pgl)
cmake_minimum_required(VERSION 2.6)

SET(PGLVERSION 2.0~pre2)

if(NOT NOPGLD)
    add_subdirectory(pgld)
endif(NOT NOPGLD)

if(NOT NOPGLCMD)
    add_subdirectory(pglcmd)
endif(NOT NOPGLCMD)

if(NOT NOPGLGUI)
    add_subdirectory(pgl-gui)
endif(NOT NOPGLGUI)
