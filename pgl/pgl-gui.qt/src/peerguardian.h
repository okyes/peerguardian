 
/***************************************************************************
 *   Copyright (C) 2007-2008 by Dimitris Palyvos-Giannas   *
 *   jimaras@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef PEERGUARDIAN_H
#define PEERGUARDIAN_H

#include <QMainWindow>
#include <QWidget>
#include <QCloseEvent>
#include <QVariant>
#include <QByteArray>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QPushButton>
#include <QIcon>
#include <QMenu>
#include <QCursor>
#include <QMessageBox>
#include <QFile>
#include <QFont>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QDate>
#include <QTime>
#include <QFileDialog>
#include <QStringList>

#include "ui_main_window.h"

#include "file_transactions.h"
#include "peerguardian_log.h"
#include "pgl_settings.h"
#include "settings_manager.h"
#include "pgl_lists.h"
#include "peerguardian_info.h"
#include "pglcmd.h"
#include "super_user.h"

#include "settings.h"
#include "whois.h"
#include "list_add.h"
#include "status_dialog.h"
#include "ip_remove.h"
#include "add_exception_dialog.h"
#include "pgl_whitelist.h"


#define VERSION_NUMBER "0.1"

//Time related defines
#define FAST_TIMER_INTERVAL 500
#define MEDIUM_TIMER_INTERVAL 2000
#define SLOW_TIMER_INTERVAL 5000
#define INFOMSG_DELAY 5000

//Log related defines
#define LOG_LIST_FONT "Times"
#define LOG_LIST_FONT_SIZE 10
#define LOG_MSG "LOG_MSG"

//File related defines
#define TEMP_SETTINGS_FILE "/tmp/temp_blockcontrol.conf"
#define TEMP_LIST_FILE "/tmp/temp_blocklists.list"

//Icon related defines
#define LOG_LIST_INFO_ICON ":/images/info.png"
#define LOG_LIST_OUTOING_ICON ":/images/outgoing.png"
#define LOG_LIST_INCOMING_ICON ":/images/incoming.png"
#define LOG_LIST_FORWARD_ICON ":/images/forward.png"
#define LOG_LIST_ERROR_ICON ":/images/error.png"
#define TRAY_ICON ":/images/tray.png"
#define TRAY_DISABLED_ICON ":/images/tray_disabled.png"
#define ENABLED_ICON ":/images/ok.png"
#define DISABLED_ICON ":/images/cancel.png"
#define RUNNING_ICON ":/images/ok.png"
#define NOT_RUNNING_ICON ":/images/cancel.png"
#define ICON_WIDTH 24
#define ICON_HEIGHT 24

#define MAINWINDOW_WIDTH 480
#define MAINWINDOW_HEIGHT 600

#define UNCHECKED 0
#define CHECKED 2

typedef enum { LOG_TIME_COLUMN, 
	LOG_NAME_COLUMN, 
	LOG_IP_COLUMN,	
	LOG_TYPE_COLUMN };

typedef enum { SETTINGS_IPFILTER_DAT,
	SETTINGS_P2B,
	SETTINGS_P2P };
    

class GuiOptions;
    
/**
*
* @short Class representing the main window of the program. Handles everything that has to do with the GUI.
*
*/

class Peerguardian : public QMainWindow, private Ui::MainWindow {

	Q_OBJECT

    QSettings *m_ProgramSettings;
    QString m_Loaded_RootFile;
    SuperUser *m_Root;
    PeerguardianInfo *m_Info;
    PeerguardianLog *m_Log;
    PeerguardianList *m_List;
    PglWhitelist *m_Whitelist;
    PglCmd *m_Control;
    QSystemTrayIcon *m_Tray;
    QMenu *m_TrayMenu;
    //Timers
    QTimer *m_FastTimer;
    QTimer *m_MediumTimer;
    QTimer *m_SlowTimer;
    
    //QList of integers with the original states for each blocklist 
    //so it's easier to track which blocklists need to be disabled or enabled
    //when the user applies the changes.
    bool m_WhitelistItemPressed;
    bool m_BlocklistItemPressed;
    bool m_treeItemPressed;
    GuiOptions *guiOptions;
    
	public:
		/**
		 * Constructor. 
		 * This function calls several other functions to do the following:
		 * a)Setup the GUI.
		 * b)Create objects of the main classes.
		 * c)Create the connections between the objects.
		 * @param parent The QWidget parent of the object. 
		 */
		Peerguardian( QWidget *parent = 0 );
		/**
		 * Destructor. Takes the appropriate actions when Peerguardian is destroyed.
		 * Deletes object pointers and saves settings.
		 */
		virtual ~Peerguardian();
        
        void getLists();
        void g_MakeConnections();
        void inicializeSettings();
		void g_SetRoot();
		void g_SetLogPath();
        void g_SetListPath();
        void g_SetControlPath();
        void g_MakeTray();
        void g_MakeMenus();
        void g_ShowAddDialog(int);
        void startTimers();
        void updateGUI();
        QList<QTreeWidgetItem*> getTreeItems(QTreeWidget *tree, int checkState=-1);
        void inicializeVariables();
        QRadioButton * getAutoListUpdateDailyRadio() {return m_AutoListUpdateDailyRadio;}
        QRadioButton * getAutoListUpdateWeeklyRadio() {return m_AutoListUpdateWeeklyRadio;}
        QRadioButton * getAutoListUpdateMonthlyRadio() {return m_AutoListUpdateMonthlyRadio;}
        QCheckBox * getAutoListUpdateBox() { return m_AutoListUpdateBox; }
        QCheckBox * getStartAtBootBox() { return m_StartAtBootBox; }
        QTreeWidget * getBlocklistTreeWidget() { return m_BlocklistTreeWidget; }
        QTreeWidget * getWhitelistTreeWidget() { return m_WhitelistTreeWidget; }
        QString getUpdateFrequencyPath();
        QString getUpdateFrequencyCurrentPath();
        
    public slots:
        void g_ShowAddExceptionDialog() { g_ShowAddDialog(ADD_MODE | EXCEPTION_MODE); };
        void g_ShowAddBlockListDialog() { g_ShowAddDialog(ADD_MODE | BLOCKLIST_MODE); };
        void g_ShowAboutDialog();
        void updateInfo();
        void g_UpdateDaemonStatus();
        void switchButtons();
        void addLogItem( LogItem item );
        void treeItemChanged(QTreeWidgetItem*, int);
        void treeItemPressed(QTreeWidgetItem* item, int column){ m_treeItemPressed = true; }
        void applyChanges();
        void startAtBoot(int);
        void updateRadioButtonToggled(bool);
        void rootFinished();

};	

#endif //PEERGUARDIAN_H

