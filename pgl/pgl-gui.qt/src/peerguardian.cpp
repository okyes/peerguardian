 
#include "peerguardian.h"
#include <QDebug>
#include "file_transactions.h"

Peerguardian::Peerguardian( QWidget *parent ) :
	QMainWindow( parent ) 

{

	setupUi( this );

    inicializeSettings();
    startTimers();
    g_MakeConnections();
    getLists();
    g_MakeTray();
    g_MakeMenus();
    updateInfo();
    
	/*
	//Restore the window's previous state
	resize( m_ProgramSettings->value( "window/size", QSize( MAINWINDOW_WIDTH, MAINWINDOW_HEIGHT ) ).toSize() ); 
	restoreGeometry( m_ProgramSettings->value("window/geometry").toByteArray() );
	restoreState( m_ProgramSettings->value( "window/state" ).toByteArray() );*/
	
}

Peerguardian::~Peerguardian() {
	
	//Save column sizes
	/*for ( int i = 0; i < m_LogTreeWidget->columnCount() - 1; i++ ) {
		QString settingName = "log/column_";
		settingName += QVariant( i ).toString();
		m_ProgramSettings->setValue( settingName, m_LogTreeWidget->columnWidth( i ) );
	}

	//Save window settings
	m_ProgramSettings->setValue( "window/state", saveState() );
	m_ProgramSettings->setValue( "window/size", size() );
	m_ProgramSettings->setValue( "window/geometry", saveGeometry() );
	m_ProgramSettings->setValue( "settings/autoscroll", m_ListAutoScroll );
	m_ProgramSettings->setValue( "settings/slow_timer", m_SlowTimer->interval() );
	m_ProgramSettings->setValue( "settings/medium_timer", m_MediumTimer->interval() );
	m_ProgramSettings->setValue( "settings/fast_timer", m_FastTimer->interval() );
	
	//Free memory
	delete m_Settings;*/
    
    qWarning() << "~Peerguardian()";
    
    if ( m_ProgramSettings != NULL )
        delete m_ProgramSettings;
    if ( m_Root != NULL )
        delete m_Root;
    if ( m_Log != NULL )
        delete m_Log;
    if ( m_Info != NULL )
        delete m_Info;
    if ( m_List != NULL )
        delete m_List;
    if ( m_Control != NULL )
        delete m_Control;
    
    delete m_FastTimer;
    delete m_MediumTimer;
    delete m_SlowTimer;
}

void Peerguardian::startTimers()
{
    //Intiallize the fast generic timer
	m_FastTimer = new QTimer;
	m_FastTimer->setInterval( m_ProgramSettings->value( "settings/fast_timer",FAST_TIMER_INTERVAL ).toInt());
	m_FastTimer->start();
	//Intiallize the medium timer for less usual procedures
	m_MediumTimer = new QTimer;
	m_MediumTimer->setInterval( m_ProgramSettings->value("settings/medium_timer",MEDIUM_TIMER_INTERVAL ).toInt() );
	m_MediumTimer->start();
	//Intiallize the slow timer for less usual procedures
	m_SlowTimer = new QTimer;
	m_SlowTimer->setInterval( m_ProgramSettings->value("settings/slow_timer", SLOW_TIMER_INTERVAL ).toInt() );
	m_SlowTimer->start();
}

void Peerguardian::g_MakeConnections()
{
	//Log tab connections
    if ( m_Log != NULL )
        connect( m_Log, SIGNAL( newItem( LogItem ) ), this, SLOT( addLogItem( LogItem ) ) );
        
	connect( m_LogTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( logTab_HandleLogChange() ) );
	connect( m_LogClearButton, SIGNAL( clicked() ), m_LogTreeWidget, SLOT( clear() ) );
	connect( m_LogClearButton, SIGNAL( clicked() ), m_Log, SLOT( clear() ) );
	
    connect( m_addExceptionButton, SIGNAL(clicked()), this, SLOT(g_ShowAddExceptionDialog()) );
    connect( m_addBlockListButton, SIGNAL(clicked()), this, SLOT(g_ShowAddBlockListDialog()) );
    
    connect( m_Log, SIGNAL( newItem( LogItem ) ), this, SLOT( logTab_AddLogItem( LogItem ) ) );
    
    //Menu related
    connect( a_Exit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
    connect( a_AboutDialog, SIGNAL( triggered() ), this, SLOT( g_ShowAboutDialog() ) );
    
    //Control related
    if ( m_Control != NULL )
    {
        connect( m_startPglButton, SIGNAL( clicked() ), m_Control, SLOT( start() ) );
        connect( m_stopPglButton, SIGNAL( clicked() ), m_Control, SLOT( stop() ) );
        connect( m_restartPglButton, SIGNAL( clicked() ), m_Control, SLOT( restart() ) );
    }
    
    //Timers
    //if ( m_Log != NULL )
        //connect( m_FastTimer, SIGNAL( timeout() ), m_Log, SLOT( update() ) );
        
    connect( m_MediumTimer, SIGNAL( timeout() ), this, SLOT( g_UpdateDaemonStatus() ) );
	connect( m_MediumTimer, SIGNAL( timeout() ), this, SLOT( updateInfo() ) );
    
    //status bar
	connect( m_Control, SIGNAL( actionMessage( QString, int ) ), m_StatusBar, SLOT( showMessage( QString, int ) ) );
	connect( m_Control, SIGNAL( finished() ), this, SLOT( switchButtons() ) );
}


void Peerguardian::getLists()
{
    if ( m_List == NULL )
        return;
 
    QStringList item_info;

    //get information about the lists being used
    foreach(ListItem* log_item, m_List->getItems()){
        item_info << log_item->name() << "BlockList";
        
        QTreeWidgetItem * tree_item = new QTreeWidgetItem(m_ListTreeWidget, item_info);
        
        if ( log_item->isEnabled() )
            tree_item->setCheckState(0, Qt::Checked);
        else if ( log_item->isDisabled() )
            tree_item->setCheckState(0, Qt::Unchecked);
            
        m_ListTreeWidget->addTopLevelItem( tree_item );
        item_info.clear();
    }
}

void Peerguardian::inicializeSettings()
{
	//Intiallize all pointers to NULL before creating the objects with g_Set**Path
	/*
	m_Settings = NULL;*/
    m_List = NULL;
	m_Log = NULL;
	m_Info = NULL;
	m_Root = NULL;
    m_Control = NULL;

    m_ProgramSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "PeerGuardian", "peerguardian"); 
    
    g_SetRoot();
    g_SetLogPath();
    g_SetListPath();
    g_SetControlPath();
    
    
}

void Peerguardian::g_SetRoot( ) {

    QString filepath = PeerguardianLog::getFilePath(m_ProgramSettings->value( "paths/super_user" ).toString());
    
    if ( ! filepath.isEmpty() )
    {
        if ( m_Root == NULL ) 
            m_Root = new SuperUser(filepath);
            
        if ( m_ProgramSettings->value( "paths/super_user" ).toString() != m_Root->getRootPath() )
            m_ProgramSettings->setValue( "paths/super_user", m_Root->getRootPath() );
    }
    else
        QMessageBox::warning( this, tr( "Could not locate sudo front-end" ), tr( "Could not find gksu or kdesu executable. The program will not be able to perform any operation which requires super user privilleges.\n\nYou can manually change the path for the sudo front-end through mobloquer's settings dialog." ), QMessageBox::Ok );
            
}

void Peerguardian::g_SetLogPath() {

    QString filepath = PeerguardianLog::getFilePath(m_ProgramSettings->value( "paths/log" ).toString());

    if ( ! filepath.isEmpty() )
    {
        if ( m_Log == NULL )
            m_Log = new PeerguardianLog(filepath);
            
        //Save the new path to the QSettings object.
        if ( m_ProgramSettings->value( "paths/log" ).toString() != m_Log->getLogPath() )
            m_ProgramSettings->setValue( "paths/log", m_Log->getLogPath() );
            
        if ( m_Info == NULL )
            m_Info = new PeerguardianInfo(m_Log->getLogPath());
    }
    //else
    //    QMessageBox::warning( this, tr( "Log file not found!" ), tr( "Peerguardian's log file was NOT found." ), QMessageBox::Ok );

	//logTab_Init();
	//manageTab_Init();
}

void Peerguardian::g_SetListPath() 
{
    QString filepath = PeerguardianList::getFilePath(m_ProgramSettings->value( "paths/list" ).toString());

    if ( ! filepath.isEmpty() )
    {
        if ( m_List == NULL ) 
            m_List = new PeerguardianList(filepath);

        if ( m_ProgramSettings->value( "paths/list" ).toString() != m_List->getListPath() )
            m_ProgramSettings->setValue( "paths/list", m_List->getListPath() );
    }

	//listTab_Init();
    
}


void Peerguardian::g_SetControlPath() 
{
    QString filepath = PglCmd::getFilePath(m_ProgramSettings->value( "paths/control" ).toString());

    if ( ! filepath.isEmpty() )
    {
        if ( m_Control == NULL ){
			m_Control = new PglCmd;
            m_Control->setFilePath(filepath, true);
        }
            
        if ( m_ProgramSettings->value( "paths/control" ).toString() != m_Control->getPath() ) 
			m_ProgramSettings->setValue( "paths/control", m_Control->getPath() );
    }

	//manageTab_Init();
}

void Peerguardian::g_ShowAddDialog(int openmode) {
	AddExceptionDialog *dialog = new AddExceptionDialog( this, openmode );
    
    dialog->exec();
    
	/*if ( dialog->exec() == QDialog::Accepted && dialog->isSettingChanged() ) {
		emit g_SettingChanged();
	}
	else {
		g_ReloadSettings();
		m_SettingChanged = false;
	}*/

	delete dialog;
}


void Peerguardian::g_MakeTray() {

	m_Tray = new QSystemTrayIcon( QIcon( TRAY_ICON ) );
	m_Tray->setVisible( true );
	g_UpdateDaemonStatus();
}


void Peerguardian::g_UpdateDaemonStatus() {

    if ( m_Info == NULL )
        return;
	m_Info->updateDaemonState();
	bool running = m_Info->daemonState();
	static QString lastIcon;

	if ( ! running ) {
        m_controlPglButtons->setCurrentIndex(0);
		//Update the label and the icon in manage tab
		m_DaemonStatusLabel->setText( tr( "<FONT COLOR=\"#FF0000\">Pgld is not running</FONT>" ) );
		m_StatusIcon->setPixmap( QIcon( NOT_RUNNING_ICON ).pixmap( ICON_WIDTH, ICON_HEIGHT ) );
		//Update the tray
		if ( lastIcon != TRAY_DISABLED_ICON ) {
			m_Tray->setIcon( QIcon( TRAY_DISABLED_ICON ) );
			lastIcon = TRAY_DISABLED_ICON;
		}
		m_Tray->setToolTip( tr( "Pgld is not running" ) );

	}
	else 
    {
        m_controlPglButtons->setCurrentIndex(1);
		QString message = tr( "<FONT COLOR=\"#008000\">Pgld is up and running</FONT>" );
		m_DaemonStatusLabel->setText( message );
		m_StatusIcon->setPixmap( QIcon( RUNNING_ICON ).pixmap( ICON_WIDTH, ICON_HEIGHT ) );
        
		if ( lastIcon != TRAY_ICON ) {
			m_Tray->setIcon( QIcon( TRAY_ICON ) );
			lastIcon = TRAY_ICON;
		}
		m_Tray->setToolTip( tr( "Pgld is up and running" ) );

	}
}

void Peerguardian::g_MakeMenus() {


	m_TrayMenu = new QMenu();
	m_TrayMenu->addAction( a_Start );
	m_TrayMenu->addAction( a_Stop );
	m_TrayMenu->addAction( a_Restart );
	m_TrayMenu->addSeparator();
	m_TrayMenu->addAction( a_AllowHttp );
	m_TrayMenu->addAction( a_AllowHttps );
	m_TrayMenu->addAction( a_AllowFtp );
	m_TrayMenu->addSeparator();
	m_TrayMenu->addAction( a_RestoreWindow );
	m_TrayMenu->addAction( a_Exit );

	//g_UpdateTrayActions();

	//a_ToggleAutoScrolling->setChecked( m_ListAutoScroll );

}

void Peerguardian::g_ShowAboutDialog() {

    QString message;
    message += QString("<b><i>Peerguardian Qt version %1</b><br>A graphical user interface for Peerguardian<br><br>").arg( VERSION_NUMBER );
    message += "Copyright (C) 2007-2008 Dimitris Palyvos-Giannas<br>";
    message += "Copyright (C) 2011 Carlos Pais (freemind@lavabit.com) <br><br>";
    message += "This program is licenced under the GNU General Public Licence v3<br><br><font size=2>";
    message +="Using modified version of the crystal icon theme:<br>http://www.everaldo.com/<br>http://www.yellowicon.com/<br><br>";
    message += "Credits go to Morpheus, jre, TheBlackSun, Pepsi_One and siofwolves from phoenixlabs.org for their help and suggestions. <br>";
    message += "I would also like to thank Art_Fowl from e-pcmag.gr for providing valuable help with Qt4 and for helping me with the project's development. <br>";
    message += "Special credit goes to Evangelos Foutras for developing the project's website, <a href=http://mobloquer.sourceforge.net>mobloquer.foutrelis.com</a></font></i>";
    
	QMessageBox::about( this, tr( "About Peerguardian Qt" ), tr( message.toUtf8() ));

}

void Peerguardian::updateInfo() {

    
    if ( m_Info == NULL )
        return;
    
	QString lRanges = m_Info->loadedRanges();
	QString dTime = m_Info->lastUpdateTime();
    
	if ( lRanges.isEmpty() ) 
		lRanges = "N/A";
	
	if ( m_Info->daemonState() == false ) 
		lRanges = "0";
	
	if ( dTime.isNull() ) 
		dTime = "Unknown";
	

	m_LoadedRangesLabel->setText( tr( "Blocked IP ranges: %1" ).arg( lRanges ) );
	m_LastUpdateLabel->setText( tr( "Last blocklist update: %1" ).arg( dTime ) );

}


void Peerguardian::switchButtons()
{
    int i = m_controlPglButtons->currentIndex();
    //i+1-i*2: returns 0 if 'i' is 1 or the reverse
    m_controlPglButtons->setCurrentIndex(i+1-i*2);
    m_StatusBar->clearMessage();
}


void Peerguardian::addLogItem( LogItem item ) {

	QString iconPath;
	QString itemTypeStr;

	if ( item.type == IGNORE || item.type == ERROR ) {
		return;
	}
	else if ( item.type == BLOCK_IN ) {
		iconPath = LOG_LIST_INCOMING_ICON;
		itemTypeStr = tr(IN_STR);
	}
	else if ( item.type == BLOCK_OUT ) {
		iconPath = LOG_LIST_OUTOING_ICON;
		itemTypeStr = tr( OUT_STR );
	}
	else if ( item.type == BLOCK_FWD ) {
		iconPath = LOG_LIST_FORWARD_ICON;
		itemTypeStr = tr( FWD_STR );
	}

	QTreeWidgetItem *newItem = new QTreeWidgetItem; 
	newItem->setText( LOG_TIME_COLUMN, item.blockTime() );
	newItem->setToolTip( LOG_TIME_COLUMN, item.blockTime() );
	newItem->setText( LOG_NAME_COLUMN, item.name() );
	newItem->setToolTip( LOG_NAME_COLUMN, item.name() );
	newItem->setText( LOG_IP_COLUMN, item.IP() );
	newItem->setToolTip( LOG_IP_COLUMN, item.IP() );
	newItem->setText( LOG_TYPE_COLUMN, itemTypeStr );
	newItem->setToolTip( LOG_TYPE_COLUMN, itemTypeStr );
	newItem->setIcon( LOG_TYPE_COLUMN, QIcon( iconPath ) );
	m_LogTreeWidget->addTopLevelItem( newItem );
		
	//Don't let the list become too big
	if ( m_LogTreeWidget->topLevelItemCount() > MAX_LOG_SIZE ) {
		m_LogTreeWidget->clearSelection();
		QTreeWidgetItem *toRem = m_LogTreeWidget->takeTopLevelItem( 0 );
		delete toRem;
		toRem = NULL;
	}

	//if ( m_ListAutoScroll == true ) 
		m_LogTreeWidget->scrollToBottom();

	//++m_BlockedConnections;

}
