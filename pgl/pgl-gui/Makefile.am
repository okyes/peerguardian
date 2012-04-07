# QT tools
RCC = @RCC@
UIC = @UIC@
MOC = @MOC@

# flags
AM_CXXFLAGS = @QT_CXXFLAGS@
AM_CPPFLAGS = \
	@QT_CPPFLAGS@ \
	-I./ui
AM_LDFLAGS = @QT_LDFLAGS@

# autotools and QT is a pain
MOCS = \
	src/moc_add_exception_dialog.cpp \
	src/moc_file_transactions.cpp \
	src/moc_gui_options.cpp \
	src/moc_peerguardian.cpp \
	src/moc_peerguardian_info.cpp \
	src/moc_pglcmd.cpp \
	src/moc_pgl_lists.cpp \
	src/moc_pgl_settings.cpp \
	src/moc_pgl_whitelist.cpp \
	src/moc_proc_thread.cpp \
	src/moc_settings.cpp \
	src/moc_super_user.cpp \
	src/moc_utils.cpp\
	src/moc_viewer_widget.cpp\
	src/moc_error_dialog.cpp
FORMSDIR = ui
FORMSHEADERS = \
	$(FORMSDIR)/ui_add_exception.h \
	$(FORMSDIR)/ui_main_window.h \
	$(FORMSDIR)/ui_settings.h

src/moc_%.cpp: src/%.h
	$(MOC) $(AM_CPPFLAGS) "$<" -o "$@"

$(FORMSDIR)/ui_%.h: $(FORMSDIR)/%.ui
	$(UIC) $< -o $@

qrc_images.cpp: images/images.qrc
	$(RCC) \
		-name images images/images.qrc -o qrc_images.cpp

$(pgl_gui_OBJECTS) : $(MOCS) $(FORMSHEADERS) qrc_images.cpp


# sources for pgl_gui binary
pgl_gui_SOURCES = \
	src/add_exception_dialog.cpp src/add_exception_dialog.h \
	src/file_transactions.cpp src/file_transactions.h \
	src/gui_options.cpp src/gui_options.h \
	src/peerguardian.cpp src/peerguardian.h \
	src/peerguardian_info.cpp src/peerguardian_info.h \
	src/pglcmd.cpp src/pglcmd.h \
	src/pgl_lists.cpp src/pgl_lists.h \
	src/pgl_settings.cpp src/pgl_settings.h \
	src/pgl_whitelist.cpp src/pgl_whitelist.h \
	src/proc_thread.cpp src/proc_thread.h \
	src/settings.cpp src/settings.h \
	src/super_user.cpp src/super_user.h \
	src/utils.cpp src/utils.h \
	src/command.cpp src/command.h \
	src/viewer_widget.cpp src/viewer_widget.h \
	src/error_dialog.cpp src/error_dialog.h\
	\
	src/main.cpp \
	qrc_images.cpp \
	$(MOCS)

pgl_gui_LDADD = @QT_LIBS@

bin_PROGRAMS = pgl_gui

# .desktop file rules
desktopdir = $(datadir)/applications
desktop_DATA = other/pgl-gui.desktop

# icon
iconsdir = $(datadir)/pixmaps
icons_DATA = images/pgl-gui.png

# for distribution, not sure on that
EXTRA_DIST = src/main.cpp \
	$(icons_DATA)
