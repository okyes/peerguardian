TEMPLATE = app
TARGET = mobloquer
DEPENDPATH += . src 
INCLUDEPATH += . src
OBJECTS_DIR = ./build/obj/
UI_DIR = ./ui
MOC_DIR = ./build/moc/

# Input
SOURCES += src/file_transactions.cpp \
           src/main.cpp \
           src/peerguardian_info.cpp \
           src/pgl_lists.cpp \
           src/peerguardian_log.cpp \
           src/pgl_settings.cpp \
           src/peerguardian.cpp\
 src/super_user.cpp \
 src/pglcmd.cpp \
 src/settings.cpp \
 src/whois.cpp \
 src/list_add.cpp \
 src/settings_manager.cpp \
 src/status_dialog.cpp \
 src/ip_whitelist.cpp \
 src/ip_remove.cpp \
 src/proc_thread.cpp\
 src/utils.cpp \
 src/add_exception_dialog.cpp\

HEADERS += src/file_transactions.h \
	src/pglcmd.h \
	src/peerguardian_info.h \
	src/pgl_lists.h \
	src/peerguardian_log.h \
	src/pgl_settings.h \
    src/peerguardian.h\    
    src/super_user.h \
 src/settings.h \
 src/whois.h \
 src/list_add.h \
 src/settings_manager.h \
 src/status_dialog.h \
 src/ip_whitelist.h \
 src/ip_remove.h \
 src/proc_thread.h\
 src/utils.h\
 src/add_exception_dialog.h


FORMS += ui/main_window.ui \
	ui/settings.ui \
	ui/whois.ui \
	ui/list_add.ui \
	ui/status_dialog.ui \
	ui/ip_whitelist.ui \
	ui/ip_remove.ui\
    ui/add_exception.ui

RESOURCES += images/images.qrc

icon.path = /usr/share/pixmaps/
icon.files = ./images/mobloquer.png

desktop.path = /usr/share/applications/
desktop.files = ./other/*.desktop

target.path = /usr/bin

INSTALLS = icon desktop target

DISTFILES += README \
 ChangeLog \
 INSTALL

CONFIG += qt debug

