
#include <QDebug>
#include <QMultiMap>
#include <QHash>
#include <QRegExp>
#include <QDBusConnection>
//#include <Action>
//#include <ActionButton>

#include "peerguardian.h"
#include "file_transactions.h"
#include "utils.h"
#include "gui_options.h"
#include "pgl_settings.h"

//using namespace PolkitQt1;
//using namespace PolkitQt1::Gui;

Peerguardian::Peerguardian( QWidget *parent) :
	QMainWindow( parent )
{

	setupUi( this );
    
    m_LogTreeWidget->setContextMenuPolicy ( Qt::CustomContextMenu );
    
    PglSettings::loadSettings();
    guiOptions = new GuiOptions(this);
    inicializeSettings();
    startTimers();
    g_MakeTray();
    g_MakeMenus();
    g_MakeConnections();
    updateInfo();
    updateGUI();
    
    //resize columns in log view
    QHeaderView * header = m_LogTreeWidget->header();
    header->resizeSection(0, header->sectionSize(0) / 1.5 );
    header->resizeSection(1, header->sectionSize(0) * 3 );
    header->resizeSection(3, header->sectionSize(0) / 1.4 );
    header->resizeSection(5, header->sectionSize(0) / 1.4 );
    header->resizeSection(6, header->sectionSize(6) / 2);
    
    m_treeItemPressed = false;
    m_StopLogging = false;
    
    a_whitelist15 = new QAction("Allow for 15min", this);
    a_whitelist60 = new QAction("Allow for 60min", this);
    a_whitelistPerm = new QAction("Allow for permanently", this);
    
    m_ConnectType["OUT"] = tr("Outgoing"); 
    m_ConnectType["IN"] = tr("Incoming");
    m_ConnectType["FWD"] = tr("Forward");
    
    m_ConnectIconType[tr("Outgoing")] = QIcon(LOG_LIST_OUTGOING_ICON);
    m_ConnectIconType[tr("Incoming")] = QIcon(LOG_LIST_INCOMING_ICON);
    m_ConnectIconType[tr("Forward")] = QIcon();
    

    QDBusConnection connection (QDBusConnection::systemBus());
    QString service("");
    QString name("pgld_message");
    QString path("/org/netfilter/pgl");
    QString interface("org.netfilter.pgl");
    
    qDebug() << connection.connect(service, path, interface, name, qobject_cast<QObject*>(this), SLOT(addLogItem(QString)));

    
    //ActionButton *bt;
    //bt = new ActionButton(kickPB, "org.qt.policykit.examples.kick", this);
    //bt->setText("Kick... (long)");

	/*
	//Restore the window's previous state
	resize( m_ProgramSettings->value( "window/size", QSize( MAINWINDOW_WIDTH, MAINWINDOW_HEIGHT ) ).toSize() );
	restoreGeometry( m_ProgramSettings->value("window/geometry").toByteArray() );
	restoreState( m_ProgramSettings->value( "window/state" ).toByteArray() );*/

}

void Peerguardian::addLogItem(QString itemString)
{
    if ( m_StopLogging )
        return;
    
    qDebug() << itemString;
    
    if ( itemString.contains("INFO:") && itemString.contains("Blocklist") )
    {
        
        QStringList parts = itemString.split(" ", QString::SkipEmptyParts);
        m_Info->setLoadedIps(parts[3] + QString(" ") +  parts[6] + QString(" ") + parts.last());
        return;
    }
    
    if ( itemString.contains("||") )
    {
        QStringList parts = itemString.split("||", QString::SkipEmptyParts);
        QStringList firstPart = parts.first().split(" ", QString::SkipEmptyParts);
        QString connectType, srcip, destip, srcport, destport;
        
        if ( firstPart.first().contains(":") )
            connectType = m_ConnectType[firstPart.first().split(":")[0]];
        else
            connectType = m_ConnectType[firstPart.first()];
            
        if ( firstPart[3] == "TCP" || firstPart[3] == "UDP" )
        {
            srcip = firstPart[1].split(":", QString::SkipEmptyParts)[0];
            srcport = firstPart[1].split(":", QString::SkipEmptyParts)[1];
            destip = firstPart[2].split(":", QString::SkipEmptyParts)[0];
            destport = firstPart[2].split(":", QString::SkipEmptyParts)[1];
        }
        else
        {
            srcip = firstPart[1];
            srcport = "";
            destip = firstPart[2];
            destport = "";
        }
            
        QStringList info;
        
        info << QTime::currentTime().toString("hh:mm:ss") << parts.last() << srcip << srcport << destip << destport << firstPart[3] << connectType;
        QTreeWidgetItem * item = new QTreeWidgetItem(m_LogTreeWidget, info);
        item->setIcon(7, m_ConnectIconType[connectType]);
        m_LogTreeWidget->addTopLevelItem(item);
        
        m_LogTreeWidget->scrollToBottom();
    }
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
    if ( m_Whitelist != NULL )
		delete m_Whitelist;
    if ( m_Control != NULL )
        delete m_Control;

    delete m_MediumTimer;
    delete m_SlowTimer;
    delete guiOptions;
}

void Peerguardian::updateGUI()
{

    if ( PglSettings::getStoredValue("INIT") == "0" )
        m_StartAtBootBox->setChecked(false);
    else if ( PglSettings::getStoredValue("INIT") == "1" )
        m_StartAtBootBox->setChecked(true);


    if ( PglSettings::getStoredValue("CRON") == "0" )
        m_AutoListUpdateBox->setChecked(false);
    else
    {
        QString frequency = getUpdateFrequencyCurrentPath();

        if ( ! frequency.isEmpty() )
        {
            m_AutoListUpdateBox->setChecked(true);
            if (frequency.contains("daily/", Qt::CaseInsensitive))
                m_AutoListUpdateDailyRadio->setChecked(true);
            else if ( frequency.contains("weekly/", Qt::CaseInsensitive))
                m_AutoListUpdateWeeklyRadio->setChecked(true);
            else if ( frequency.contains("monthly/", Qt::CaseInsensitive))
                m_AutoListUpdateMonthlyRadio->setChecked(true);
        }
    }

    updateBlocklist();
    updateWhitelist();
    guiOptions->update();
}

void Peerguardian::startTimers()
{
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
    {
        connect( m_Log, SIGNAL( newItem( LogItem ) ), this, SLOT( addLogItem( LogItem ) ) );
        connect( m_LogTreeWidget, SIGNAL(customContextMenuRequested ( const QPoint &)), this, SLOT(showLogRightClickMenu(const QPoint &)));
    }
       
    connect(m_StopLoggingButton, SIGNAL(clicked()), this, SLOT(startStopLogging()));

	//connect( m_LogTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( logTab_HandleLogChange() ) );
	connect( m_LogClearButton, SIGNAL( clicked() ), m_LogTreeWidget, SLOT( clear() ) );
	connect( m_LogClearButton, SIGNAL( clicked() ), m_Log, SLOT( clear() ) );

    connect( m_addExceptionButton, SIGNAL(clicked()), this, SLOT(g_ShowAddExceptionDialog()) );
    connect( m_addBlockListButton, SIGNAL(clicked()), this, SLOT(g_ShowAddBlockListDialog()) );

    connect( m_Log, SIGNAL( newItem( LogItem ) ), this, SLOT( addLogItem( LogItem ) ) );

    //Menu related
    connect( a_Exit, SIGNAL( triggered() ), this, SLOT( quit() ) );
    connect( a_AboutDialog, SIGNAL( triggered() ), this, SLOT( g_ShowAboutDialog() ) );

    //Control related
    if ( m_Control != NULL )
    {
        connect( m_startPglButton, SIGNAL( clicked() ), m_Control, SLOT( start() ) );
        connect( m_stopPglButton, SIGNAL( clicked() ), m_Control, SLOT( stop() ) );
        connect( m_restartPglButton, SIGNAL( clicked() ), m_Control, SLOT( restart() ) );
        connect( m_updatePglButton, SIGNAL( clicked() ), m_Control, SLOT( update() ) );
    }

    connect( m_MediumTimer, SIGNAL( timeout() ), this, SLOT( g_UpdateDaemonStatus() ) );
	connect( m_MediumTimer, SIGNAL( timeout() ), this, SLOT( updateInfo() ) );

    //status bar
	connect( m_Control, SIGNAL( actionMessage( QString, int ) ), m_StatusBar, SLOT( showMessage( QString, int ) ) );
	connect( m_Control, SIGNAL( finished() ), m_StatusBar, SLOT( clearMessage() ) );

	//Blocklist and Whitelist Tree Widgets
	connect(m_WhitelistTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(treeItemChanged(QTreeWidgetItem*, int)));
	connect(m_BlocklistTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(treeItemChanged(QTreeWidgetItem*, int)));
	connect(m_WhitelistTreeWidget, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(treeItemPressed(QTreeWidgetItem*, int)));
	connect(m_BlocklistTreeWidget, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(treeItemPressed(QTreeWidgetItem*, int)));

    /********************************Configure tab****************************/
    connect(m_UndoButton, SIGNAL(clicked()), this, SLOT(undoGuiOptions()));
    connect(m_ApplyButton, SIGNAL(clicked()), this, SLOT(applyChanges()));
    connect(m_StartAtBootBox, SIGNAL(clicked(bool)), this, SLOT(checkboxChanged(bool)));
    connect(m_AutoListUpdateBox, SIGNAL(clicked(bool)), this, SLOT(checkboxChanged(bool)));


    //connect update frequency radio buttons
    connect(m_AutoListUpdateDailyRadio, SIGNAL(clicked(bool)), this, SLOT(updateRadioButtonToggled(bool)));
    connect(m_AutoListUpdateWeeklyRadio, SIGNAL(clicked(bool)), this, SLOT(updateRadioButtonToggled(bool)));
    connect(m_AutoListUpdateMonthlyRadio, SIGNAL(clicked(bool)), this, SLOT(updateRadioButtonToggled(bool)));

    if ( m_Root )
        connect(m_Root, SIGNAL(finished()), this, SLOT(rootFinished()));

    //connect the remove buttons
    connect(m_rmBlockListButton, SIGNAL(clicked()), this, SLOT(removeListItems()));
    connect(m_rmExceptionButton, SIGNAL(clicked()), this, SLOT(removeListItems()));
    
    //tray iconPath
    connect(m_Tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayIconClicked(QSystemTrayIcon::ActivationReason)));

    connect(a_SettingsDialog, SIGNAL(triggered()), this, SLOT(openSettingsDialog()));
}

void Peerguardian::quit()
{
    int answer;
    
    if (m_ApplyButton->isEnabled())
    {
        answer = confirm(tr("Really quit?"), tr("You have <b>unapplied</b> changes, do you really want to quit?"), this);
        
        if ( answer == QMessageBox::No )
        {
            return;
        }
    }
    
    m_App->quit();
}

void Peerguardian::addApp(QApplication & app)
{
    m_App = &app;
}

void Peerguardian::closeEvent ( QCloseEvent * event )
{
    this->hide();
}

void Peerguardian::onTrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    if ( reason == QSystemTrayIcon::Trigger )
        this->setVisible ( ! this->isVisible() );
}


void Peerguardian::removeListItems()
{
    QList<QTreeWidgetItem *> treeItems;
    QTreeWidget * tree;
    int i;
    bool whitelist;

    if ( sender()->objectName().contains("block", Qt::CaseInsensitive) )
    {
        tree  = m_BlocklistTreeWidget;
        whitelist = false;
    }
    else
    {
        tree  = m_WhitelistTreeWidget;
        whitelist = true;
    }

    foreach(QTreeWidgetItem *item, tree->selectedItems())
    {
        i = tree->indexOfTopLevelItem(item);
        tree->takeTopLevelItem(i);

        if ( whitelist )
        {
            guiOptions->addRemovedWhitelistItem(item);
        }
    }

    m_ApplyButton->setEnabled(guiOptions->isChanged());
    m_UndoButton->setEnabled(m_ApplyButton->isEnabled());
}

void Peerguardian::rootFinished()
{
    
    if ( m_FilesToMove.isEmpty() )
        return;
        
    QString pglcmd_conf = PglSettings::getStoredValue("CMD_CONF").split("/").last();
    QString blocklists_list = PglSettings::getStoredValue("BLOCKLISTS_LIST").split("/").last();
    QString tmp_pglcmd = QString("/tmp/%1").arg(pglcmd_conf);
    QString tmp_blocklists = QString("/tmp/%1").arg(blocklists_list);

    if ( ( m_FilesToMove.contains(tmp_pglcmd) && QFile::exists(tmp_pglcmd) ) ||  
        (m_FilesToMove.contains(tmp_blocklists) && QFile::exists(tmp_blocklists)))
    {
        m_ApplyButton->setEnabled(true);
    }
    else
    {
        m_Whitelist->updateSettings(getTreeItems(m_WhitelistTreeWidget));
        m_Whitelist->load();
        PglSettings::loadSettings();
        updateGUI();
        m_ApplyButton->setEnabled(false);
    }
    
    m_FilesToMove.clear();
    m_UndoButton->setEnabled(m_ApplyButton->isEnabled());
}

void Peerguardian::updateRadioButtonToggled(bool toggled)
{
    QRadioButton * item = qobject_cast<QRadioButton*>(sender());
     
    m_ApplyButton->setEnabled(guiOptions->isChanged());
    m_UndoButton->setEnabled(m_ApplyButton->isEnabled());
    
    if ( guiOptions->hasRadioButtonChanged(item) )
    {
        item->setIcon(QIcon(WARNING_ICON));
        item->setStatusTip(tr("You need to click the Apply button so the changes take effect"));
    }
    else
    {    
        item->setIcon(QIcon());
        item->setStatusTip("");
    }
    
    if ( item->objectName() != m_AutoListUpdateDailyRadio->objectName() )
    {
        m_AutoListUpdateDailyRadio->setIcon(QIcon());
        m_AutoListUpdateDailyRadio->setStatusTip("");
    }
    	
    if ( item->objectName() != m_AutoListUpdateWeeklyRadio->objectName() )
    {
        m_AutoListUpdateWeeklyRadio->setIcon(QIcon());
        m_AutoListUpdateWeeklyRadio->setStatusTip("");
    }
	
    if ( item->objectName() != m_AutoListUpdateMonthlyRadio->objectName() )
    {
        m_AutoListUpdateMonthlyRadio->setIcon(QIcon());
        m_AutoListUpdateMonthlyRadio->setStatusTip("");
    }
	
    
}


void Peerguardian::checkboxChanged(bool state)
{
    m_ApplyButton->setEnabled(guiOptions->isChanged());
    m_UndoButton->setEnabled(m_ApplyButton->isEnabled());
    
    QCheckBox *item = qobject_cast<QCheckBox*>(sender());

    
    if ( guiOptions->hasCheckboxChanged(item) )
    {
        item->setIcon(QIcon(WARNING_ICON));
        item->setStatusTip(tr("You need to click the Apply button so the changes take effect"));
    }
    else
    {
        item->setIcon(QIcon());
        item->setStatusTip("");
    }
}



QString Peerguardian::getUpdateFrequencyPath()
{
    QString path("/etc/cron.");
    QString script ("pglcmd");

    if ( m_AutoListUpdateDailyRadio->isChecked() )
        return path += "daily/" + script;
    else if ( m_AutoListUpdateWeeklyRadio->isChecked() )
        return path += "weekly/" + script;
    else if ( m_AutoListUpdateMonthlyRadio->isChecked() )
        return path += "monthly/" + script;

    return QString("");
}


QString Peerguardian::getUpdateFrequencyCurrentPath()
{
    QString path("/etc/cron.");
    QString script ("pglcmd");
    QStringList times;
    times << "daily/" << "weekly/" << "monthly/";;


    foreach(QString time, times)
        if ( QFile::exists(path + time + script ) )
            return path + time + script;

    return QString("");
}


void Peerguardian::applyChanges()
{

    QMap<QString, QString> filesToMove;
    QStringList pglcmdConf;
    bool updatePglcmdConf = guiOptions->hasToUpdatePglcmdConf();
    bool updateBlocklistsList = guiOptions->hasToUpdateBlocklistList();
    QString filepath;
    
    //only apply IPtables commands if the daemon is running
    if ( m_Info->daemonState() )
    {
        //apply new changes directly in iptables
        QStringList iptablesCommands = m_Whitelist->updateWhitelistItemsInIptables(getTreeItems(m_WhitelistTreeWidget), guiOptions);
        if ( ! iptablesCommands.isEmpty() )
            m_Root->executeCommands(iptablesCommands, false);
    }
    
    //================ update /etc/pgl/pglcmd.conf ================/
	if ( updatePglcmdConf )
    {
        //======== Whitelisted IPs are stored in pglcmd.conf too ==========/
        pglcmdConf = m_Whitelist->updatePglcmdConf(getTreeItems(m_WhitelistTreeWidget));

        //========start at boot option ( pglcmd.conf )==========/
        QString value (QString::number(int(m_StartAtBootBox->isChecked())));
        if ( hasValueInData("INIT", pglcmdConf) || PglSettings::getStoredValue("INIT") != value )
            pglcmdConf = replaceValueInData(pglcmdConf, "INIT", value);

        //====== update  frequency check box ( pglcmd.conf ) ==========/
        value = QString::number(int(m_AutoListUpdateBox->isChecked()));
        if ( hasValueInData("CRON", pglcmdConf) || PglSettings::getStoredValue("CRON") != value )
            pglcmdConf = replaceValueInData(pglcmdConf, "CRON", value);

        //add /tmp/pglcmd.conf to the filesToMove
        filesToMove["/tmp/" + getFileName(PGLCMD_CONF_PATH)] = PGLCMD_CONF_PATH;
        saveFileData(pglcmdConf, "/tmp/" + getFileName(PGLCMD_CONF_PATH));
    }

    //================ update /etc/pgl/blocklists.list ================/
    if ( updateBlocklistsList )
    {
        m_List->update(getTreeItems(m_BlocklistTreeWidget));
        filepath = "/tmp/" + m_List->getListPath().split("/").last();
        if ( QFile::exists(filepath) ) //update the blocklists.list file
            filesToMove[filepath] = m_List->getListPath();
    }

    //================ manage the local blocklists ====================/
    QHash<QString, bool> localFiles = m_List->getLocalLists();
    QString localBlocklistDir = m_List->getLocalBlocklistDir();

    foreach(QString filepath, localFiles.keys())
    {
        //if local blocklist is active
        if( localFiles[filepath] )
        {
            QString symFilepath = "/tmp/" + filepath.split("/").last();

            //if symlink to the file doesn't exist, create it
            if ( ! isPointingTo(localBlocklistDir, filepath) )
            {
                QFile::link(filepath, symFilepath);
                filesToMove[symFilepath] = localBlocklistDir;
            }
        }
        else
        {
            QString symFilepath = getPointer(localBlocklistDir, filepath);

            if ( ! symFilepath.isEmpty() ) //if symlink exists, delete it
                filesToMove[symFilepath] = "/dev/null"; //temporary hack
        }
    }


    //====== update  frequency radio buttons ==========/
    filepath = getUpdateFrequencyPath();
    if ( ! QFile::exists(filepath) )
        filesToMove[getUpdateFrequencyCurrentPath()] = filepath;
        
    m_FilesToMove = filesToMove.keys();

    if ( ! filesToMove.isEmpty() )
        m_Root->moveFiles(filesToMove, false);

    m_Root->executeAll();
    
    m_Whitelist->updateSettings(getTreeItems(m_WhitelistTreeWidget), guiOptions->getPositionFirstAddedWhitelistItem(), false);
    guiOptions->updateWhitelist(guiOptions->getPositionFirstAddedWhitelistItem());
    m_ApplyButton->setEnabled(false); //assume changes will be applied, if not this button will be enabled afterwards
    m_UndoButton->setEnabled(false);
}


QList<QTreeWidgetItem*> Peerguardian::getTreeItems(QTreeWidget *tree, int checkState)
{
	QList<QTreeWidgetItem*> items;

	for (int i=0; i < tree->topLevelItemCount(); i++ )
		if ( tree->topLevelItem(i)->checkState(0) == checkState || checkState == -1)
			items << tree->topLevelItem(i);

	return items;
}


void Peerguardian::treeItemChanged(QTreeWidgetItem* item, int column)
{
    if ( ! m_treeItemPressed )
        return;

    m_treeItemPressed = false;

    m_ApplyButton->setEnabled(guiOptions->isChanged());
    m_UndoButton->setEnabled(m_ApplyButton->isEnabled());
    if ( guiOptions->isChanged(item) )
    {
        item->setIcon(0, QIcon(WARNING_ICON));
        item->setStatusTip(0, tr("You need to click the Apply button so the changes take effect"));
    }
    else
    {
        item->setIcon(0, QIcon());
        item->setStatusTip(0, "");
    }

}

void Peerguardian::treeItemPressed(QTreeWidgetItem* item, int column)
{
    m_treeItemPressed = true;

    if ( item->treeWidget()->objectName().contains("block", Qt::CaseInsensitive) )
    {
        m_rmExceptionButton->setEnabled(false);
        m_rmBlockListButton->setEnabled(true);
    }
    else
    {
        m_rmExceptionButton->setEnabled(true);
        m_rmBlockListButton->setEnabled(false);
    }

}

void Peerguardian::updateBlocklist()
{
    if ( m_List == NULL )
        return;

    m_List->updateListsFromFile();

    if ( m_BlocklistTreeWidget->topLevelItemCount() > 0 )
        m_BlocklistTreeWidget->clear();

    QStringList item_info;

    //get information about the blocklists being used
    foreach(ListItem* log_item, m_List->getValidItems())
    {
        item_info << log_item->name() << log_item->location();
        QTreeWidgetItem * tree_item = new QTreeWidgetItem(m_BlocklistTreeWidget, item_info);

        if ( log_item->isEnabled() )
            tree_item->setCheckState(0, Qt::Checked);
        else
            tree_item->setCheckState(0, Qt::Unchecked);

        m_BlocklistTreeWidget->addTopLevelItem( tree_item );
        item_info.clear();
    }

    //get local blocklists
    foreach(QFileInfo blocklist, m_List->getLocalBlocklists())
    {
        item_info << blocklist.fileName() << blocklist.absoluteFilePath();
        QTreeWidgetItem * tree_item = new QTreeWidgetItem(m_BlocklistTreeWidget, item_info);
        tree_item->setCheckState(0, Qt::Checked);
        item_info.clear();
    }
}

void Peerguardian::updateWhitelist()
{
    
    QMap<QString, QStringList> items;
    QStringList values;
    QStringList info;
    
    if ( m_WhitelistTreeWidget->topLevelItemCount() > 0 )
        m_WhitelistTreeWidget->clear();
    
    //get enabled whitelisted IPs and ports
    items = m_Whitelist->getEnabledWhitelistedItems();
    foreach(QString key, items.keys())
    {
        values = items[key];
        foreach( QString value, values )
        {
            info << value << m_Whitelist->getTypeAsString(key) << m_Whitelist->getProtocol(key);
            QTreeWidgetItem * tree_item = new QTreeWidgetItem(m_WhitelistTreeWidget, info);
            tree_item->setCheckState(0, Qt::Checked );
            m_WhitelistTreeWidget->addTopLevelItem(tree_item);
            info.clear();
        }
    }

    //get disabled whitelisted IPs and ports
    items = m_Whitelist->getDisabledWhitelistedItems();
    foreach(QString key, items.keys())
    {
        values = items[key];
        foreach( QString value, values )
        {
            info << value << m_Whitelist->getTypeAsString(key) << m_Whitelist->getProtocol(key);
            QTreeWidgetItem * tree_item = new QTreeWidgetItem(m_WhitelistTreeWidget, info);
            tree_item->setCheckState(0, Qt::Unchecked );
            m_WhitelistTreeWidget->addTopLevelItem(tree_item);
            info.clear();
        }
    }
}

void Peerguardian::inicializeSettings()
{
	//Intiallize all pointers to NULL before creating the objects with g_Set==Path
	/*
	m_Settings = NULL;*/
    m_List = NULL;
    m_Whitelist = NULL;
    m_Log = NULL;
    m_Info = NULL;
    m_Root = NULL;
    m_Control = NULL;
    quitApp = false;

    m_ProgramSettings = new QSettings(QSettings::UserScope, "pgl", "pgl-gui");

    g_SetRoot();
    g_SetLogPath();
    g_SetListPath();
    g_SetControlPath();


}

void Peerguardian::g_SetRoot( ) {
    
    if ( m_Root != NULL )
        delete m_Root;
    
    QString gSudo = m_ProgramSettings->value("paths/super_user").toString();
    qDebug() << "gSudo: " << gSudo; 
    m_Root = new SuperUser(this, gSudo);
}

void Peerguardian::g_SetLogPath() {

    QString filepath = PeerguardianLog::getFilePath();
    
    if ( ! filepath.isEmpty() && m_Log == NULL )
    {
        m_Log = new PeerguardianLog();
        m_Log->setFilePath(filepath, true);

        if ( m_Info == NULL )
            m_Info = new PeerguardianInfo(PglSettings::getStoredValue("DAEMON_LOG"), this);
    }
    //else
        //QMessageBox::warning( this, tr( "Log file not found!" ), tr( "Peerguardian's log file was NOT found." ), QMessageBox::Ok );

	//logTab_Init();
	//manageTab_Init();
}

void Peerguardian::g_SetListPath()
{
    QString filepath = PeerguardianList::getFilePath();

    if ( ! filepath.isEmpty() && m_List == NULL )
            m_List = new PeerguardianList(filepath);

    //whitelisted Ips and ports - /etc/pgl/pglcmd.conf and /etc/pgl/allow.p2p and
    //$HOME/.config/pgl/pgl-gui.conf for disabled items
    m_Whitelist = new PglWhitelist(m_ProgramSettings);

}

void Peerguardian::g_SetControlPath()
{
    QString  gSudo = m_ProgramSettings->value("paths/super_user").toString();
    m_Control = new PglCmd(this, PglSettings::getStoredValue("CMD_PATHNAME"), gSudo);
    
}

void Peerguardian::g_ShowAddDialog(int openmode) {
    AddExceptionDialog *dialog = NULL;
    bool newItems = false;

    if ( openmode == (ADD_MODE | EXCEPTION_MODE) )
    {
        dialog = new AddExceptionDialog( this, openmode, getTreeItems(m_WhitelistTreeWidget));
        dialog->exec();

        foreach(WhitelistItem whiteItem, dialog->getItems())
        {
            QStringList info; info << whiteItem.value() << whiteItem.connection() << whiteItem.protocol();
            QTreeWidgetItem * treeItem = new QTreeWidgetItem(m_WhitelistTreeWidget, info);
            treeItem->setCheckState(0, Qt::Checked);
            treeItem->setIcon(0, QIcon(WARNING_ICON));
            treeItem->setStatusTip(0, tr("You need to click the Apply button so the changes take effect"));
            m_WhitelistTreeWidget->addTopLevelItem(treeItem);
            newItems = true;
        }

	}
    else if (  openmode == (ADD_MODE | BLOCKLIST_MODE) )
    {

        dialog = new AddExceptionDialog( this, openmode, getTreeItems(m_BlocklistTreeWidget));
        dialog->exec();
        QString name;

        foreach(QString blocklist, dialog->getBlocklists())
        {
            if ( QFile::exists(blocklist) )
                name = blocklist.split("/").last();
            else
                name = blocklist;

            QStringList info; info << name <<  blocklist;
            QTreeWidgetItem * treeItem = new QTreeWidgetItem(m_BlocklistTreeWidget, info);
            treeItem->setCheckState(0, Qt::Checked);
            treeItem->setIcon(0, QIcon(WARNING_ICON));
            treeItem->setStatusTip(0, tr("You need to click the Apply button so the changes take effect"));
            m_BlocklistTreeWidget->addTopLevelItem(treeItem);
            newItems = true;

        }

        m_BlocklistTreeWidget->scrollToBottom();
    }

    if ( newItems )
    {
        m_ApplyButton->setEnabled(true);
        m_UndoButton->setEnabled(m_ApplyButton->isEnabled());
    }

	/*if ( dialog->exec() == QDialog::Accepted && dialog->isSettingChanged() ) {
		emit g_SettingChanged();
	}
	else {
		g_ReloadSettings();
		m_SettingChanged = false;
	}*/

    if ( dialog != NULL )
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

        setWindowIcon(QIcon(TRAY_DISABLED_ICON));
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
        
        setWindowIcon(QIcon(TRAY_ICON));
	}
}

void Peerguardian::g_MakeMenus() {


    //tray icon menu
	m_TrayMenu = new QMenu(this);
	m_TrayMenu->addAction( a_Start );
	m_TrayMenu->addAction( a_Stop );
	m_TrayMenu->addAction( a_Restart );
	/*m_TrayMenu->addSeparator();
	m_TrayMenu->addAction( a_AllowHttp );
	m_TrayMenu->addAction( a_AllowHttps );
	m_TrayMenu->addAction( a_AllowFtp );*/
	m_TrayMenu->addSeparator();
	m_TrayMenu->addAction( a_Exit );
	m_Tray->setContextMenu(m_TrayMenu);
    

}

void Peerguardian::g_ShowAboutDialog() {

    QString message;
    message += QString("<b><i>PeerGuardian Linux version %1</b><br>A Graphical User Interface for PeerGuardian Linux<br><br>").arg( VERSION_NUMBER );
    message += "Copyright (C) 2007-2008 Dimitris Palyvos-Giannas<br>";
    message += "Copyright (C) 2011 Carlos Pais <br><br>";
    message += "pgl is licensed under the GNU General Public License v3, or (at\
                your option) any later version. This program comes with\
                ABSOLUTELY NO WARRANTY. This is free software, and you are\
                welcome to modify and/or redistribute it.<br><br><font size=2>";
    message +="Using modified version of the crystal icon theme:<br>http://www.everaldo.com/<br>http://www.yellowicon.com/<br><br>";
    message += "Credits go to Morpheus, jre, TheBlackSun, Pepsi_One and siofwolves from phoenixlabs.org for their help and suggestions. <br>";
    message += "I would also like to thank Art_Fowl from e-pcmag.gr for providing valuable help with Qt4 and for helping me with the project's development. <br>";
    message += "Special credit goes to Evangelos Foutras for developing the old project's website, <a href=http://mobloquer.sourceforge.net>mobloquer.foutrelis.com</a></font></i>";

	QMessageBox::about( this, tr( "About PeerGuardian Linux GUI" ), tr( message.toUtf8() ));

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
		iconPath = LOG_LIST_OUTGOING_ICON;
		itemTypeStr = tr( OUT_STR );
	}
	else if ( item.type == BLOCK_FWD ) {
		iconPath = LOG_LIST_FORWARD_ICON;
		itemTypeStr = tr( FWD_STR );
	}


	QStringList item_info;
	item_info << item.blockTime();
	item_info << item.name();
	item_info << item.getIpSource();
	item_info << item.getIpDest();
	item_info << "TCP (change)";
	item_info << "Blocked";

	QTreeWidgetItem *newItem = new QTreeWidgetItem(m_LogTreeWidget, item_info);

	/*newItem->setText( LOG_TIME_COLUMN, item.blockTime() );
	newItem->setToolTip( LOG_TIME_COLUMN, item.blockTime() );
	newItem->setText( LOG_NAME_COLUMN, item.name() );
	newItem->setToolTip( LOG_NAME_COLUMN, item.name() );
	newItem->setText( LOG_IP_COLUMN, item.IP() );
	newItem->setToolTip( LOG_IP_COLUMN, item.IP() );
	newItem->setText( LOG_TYPE_COLUMN, itemTypeStr );
	newItem->setToolTip( LOG_TYPE_COLUMN, itemTypeStr );
	newItem->setIcon( LOG_TYPE_COLUMN, QIcon( iconPath ) );*/
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


void Peerguardian::undoGuiOptions() 
{ 
    int answer = 0;
    
    answer = confirm(tr("Really Undo?"), tr("Are you sure you want to ignore the unsaved changes?"), this);
    
    if ( answer == QMessageBox::Yes)
    {
        guiOptions->undo(); 
        m_UndoButton->setEnabled(false);
        m_ApplyButton->setEnabled(false);
    }
}

void Peerguardian::startStopLogging()
{ 
    m_StopLogging = ! m_StopLogging;
    
    QPushButton *button = qobject_cast<QPushButton*> (sender()); 
    
    if ( m_StopLogging )
    {
        button->setIcon(QIcon(ENABLED_ICON));
        button->setText(tr("Start Logging"));
    }
    else
    {
        button->setIcon(QIcon(DISABLED_ICON));
        button->setText(tr("Stop Logging"));
    }
}

void Peerguardian::openSettingsDialog()
{
    SettingsDialog * dialog = new SettingsDialog(this);
        
    if ( dialog->exec() )
    {
        m_ProgramSettings->setValue("paths/super_user", dialog->file_GetRootPath());
        SuperUser::m_SudoCmd = m_ProgramSettings->value("paths/super_user").toString();
    }
    
    if ( dialog )
        delete dialog; 
}


void Peerguardian::showLogRightClickMenu(const QPoint& p)
{
    QTreeWidgetItem * item = m_LogTreeWidget->itemAt(p);
    
    if ( item == NULL )
        return;
    
    QMenu   menu  (this);
    QMenu *menuIp;
    QMenu *menuPort;
    
    if ( item->text(7) == "Incoming" )
    {
        menuIp = menu.addMenu("Allow IP " + item->text(2));
    }
    else
    {
        menuIp =  menu.addMenu("Allow IP " + item->text(4));
    }
    
    menuPort = menu.addMenu("Allow Port " + item->text(5));
    
    menuIp->addAction(a_whitelist15);
    menuIp->addAction(a_whitelist60);
    menuIp->addAction(a_whitelistPerm);
    
    menuPort->addAction(a_whitelist15);
    menuPort->addAction(a_whitelist60);
    menuPort->addAction(a_whitelistPerm);
    
    menu.exec(m_LogTreeWidget->mapToGlobal(p));

}
