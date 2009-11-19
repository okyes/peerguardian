#Sources
set( pgl-gui_SRCS
    debug.cpp
    filehandler.cpp
    processhandler.cpp
    abstracthandler.cpp
    loghandler.cpp
    main.cpp
    )

#Qt settings
include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
qt4_automoc(${pgl-gui_SRCS})
#Executable and linking
add_executable( pgl-gui ${pgl-gui_SRCS})
target_link_libraries(pgl-gui ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
