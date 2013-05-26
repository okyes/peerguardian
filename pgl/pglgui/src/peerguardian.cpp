
#include <QDebug>
#include <QMultiMap>
#include <QHash>
#include <QRegExp>
#include <QDBusConnection>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
//#include <Action>
//#include <ActionButton>

#include "peerguardian.h"
#include "file_transactions.h"
#include "utils.h"
#include "gui_options.h"
#include "pgl_settings.h"
#include "viewer_widget.h"
#include "error_dialog.h"

//using namespace PolkitQt1;
//using namespace PolkitQt1::Gui;

Peerguardian::Peerguardian( QWidget *parent) :
	QMainWindow( parent )
{
    mUi.setupUi( this );
    
    mUi.logTreeWidget->setContextMenuPolicy ( Qt::CustomContextMenu );
    
    if ( ! PglSettings::loadSettings() )
        QMessageBox::warning(this, tr("Error"), PglSettings::lastError(), QMessageBox::Ok);

    m_StopLogging = false;
    mAutomaticScroll = true;
    mIgnoreScroll = false;
    mTrayIconDisabled = QIcon(TRAY_DISABLED_ICON);
    mTrayIconEnabled = QIcon(TRAY_ICON);
    setWindowIcon(mTrayIconDisabled);
    
    initCore();
    startTimers();
    g_MakeTray();
    g_MakeMenus();
    g_MakeConnections();
    updateInfo();
    updateGUI();
    
    //resize columns in log view
    QHeaderView * header = mUi.logTreeWidget->header();
    header->resizeSection(0, header->sectionSize(0) / 1.5 );
    header->resizeSection(1, header->sectionSize(0) * 3 );
    header->resizeSection(3, header->sectionSize(0) / 1.4 );
    header->resizeSection(5, header->sectionSize(0) / 1.4 );
    header->resizeSection(6, header->sectionSize(6) / 2);
    
    //resize column in whitelist view
    header = mUi.whitelistTreeWidget->header();
    header->resizeSection(0, header->sectionSize(0) * 2);
    header->resizeSection(2, header->sectionSize(2) / 2);
        
    a_whitelistIpTemp = new QAction(tr("Allow temporarily"), this);
    a_whitelistIpTemp->setToolTip(tr("Allows until pgld is restarted."));
    a_whitelistIpPerm = new QAction(tr("Allow permanently"), this);
    a_whitelistPortTemp = new QAction(tr("Allow temporarily"), this);
    a_whitelistPortTemp->setToolTip(tr("Allows until pgld is restarted."));
    a_whitelistPortPerm = new QAction(tr("Allow permanently"), this);
    aWhoisIp = new QAction(tr("Whois "), this);

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
    
    bool ok = connection.connect(service, path, interface, name, qobject_cast<QObject*>(this), SLOT(addLogItem(QString)));

    if ( ! ok )
        qDebug() << "Connection to DBus failed.";
    else
        qDebug() << "Connection to DBus was successful.";

    //center window
    QDesktopWidget *desktop = QApplication::desktop();
    int yy = desktop->height()/2-height()/2;
    int xx = desktop->width() /2-width()/2;
    move(xx, yy);
    
    mUi.logTreeWidget->verticalScrollBar()->installEventFilter(this);
    
    connect(aWhoisIp, SIGNAL(triggered()), this, SLOT(onWhoisTriggered()));
    connect(a_whitelistIpTemp, SIGNAL(triggered()), this, SLOT(whitelistItem()));
    connect(a_whitelistIpPerm, SIGNAL(triggered()), this, SLOT(whitelistItem()));
    connect(a_whitelistPortTemp, SIGNAL(triggered()), this, SLOT(whitelistItem()));
    connect(a_whitelistPortPerm, SIGNAL(triggered()), this, SLOT(whitelistItem()));
    
    //ActionButton *bt;
    //bt = new ActionButton(kickPB, "org.qt.policykit.examples.kick", this);
    //bt->setText("Kick... (long)");
	
    restoreSettings();
}

Peerguardian::~Peerguardian()
{
    qWarning() << "~Peerguardian()";

    saveSettings();

    //Free memory
    if(mPglCore)
        delete mPglCore;
}

void Peerguardian::addLogItem(QString itemString)
{
    if ( m_StopLogging )
        return;
    
    if ( itemString.contains("INFO:") && itemString.contains("Blocking") )
    {
        
        QStringList parts = itemString.split("INFO:", QString::SkipEmptyParts);
        m_Info->setLoadedIps(parts[0]);
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
        
        if ( mUi.logTreeWidget->topLevelItemCount() > m_MaxLogSize ) {
            //mIgnoreScroll = true;
            mUi.logTreeWidget->takeTopLevelItem(0);
            //mIgnoreScroll = false;
        }
        
        info << QTime::currentTime().toString("hh:mm:ss") << parts.last() << srcip << srcport << destip << destport << firstPart[3] << connectType;
        QTreeWidgetItem * item = new QTreeWidgetItem(mUi.logTreeWidget, info);
        item->setIcon(7, m_ConnectIconType[connectType]);
        mUi.logTreeWidget->addTopLevelItem(item);
            
        
        if (mAutomaticScroll)
            mUi.logTreeWidget->scrollToBottom();
    }
}


void Peerguardian::saveSettings()
{
    QString name;
    //Save column sizes
    for (int i = 0; i < mUi.logTreeWidget->columnCount() ; i++ ) {
        name = QString("logTreeView/column_%1").arg(i);
        m_ProgramSettings->setValue( name, mUi.logTreeWidget->columnWidth(i) );
    }

    //Save window settings
    m_ProgramSettings->setValue( "window/state", saveState() );
    m_ProgramSettings->setValue( "window/geometry", saveGeometry() );
}

void Peerguardian::restoreSettings()
{
    bool ok;
    //Restore to the window's previous state
    if (m_ProgramSettings->contains("window/geometry"))
        restoreGeometry( m_ProgramSettings->value("window/geometry").toByteArray() );

    if (m_ProgramSettings->contains("window/state"))
        restoreState( m_ProgramSettings->value( "window/state" ).toByteArray() );

    for (int i = 0; i < mUi.logTreeWidget->columnCount(); i++ ) {
        QString settingName = "logTreeView/column_" + QString::number(i);
        if (m_ProgramSettings->contains(settingName)) {
            int value = m_ProgramSettings->value(settingName).toInt(&ok);
            if (ok)
                mUi.logTreeWidget->setColumnWidth(i, value);
        }
    }

    QString max = m_ProgramSettings->value("maximum_log_entries").toString();
    if ( max.isEmpty() ) {
        m_ProgramSettings->setValue("maximum_log_entries", MAX_LOG_SIZE);
        m_MaxLogSize = MAX_LOG_SIZE;
    }
    else {
        bool ok;
        m_MaxLogSize = max.toInt(&ok);
        if ( ! ok )
            m_MaxLogSize = MAX_LOG_SIZE;
    }
}

void Peerguardian::updateGUI()
{

    mUi.startAtBootCheckBox->setChecked(mPglCore->option("startAtBoot")->isEnabled());
    mUi.updateAutomaticallyCheckBox->setChecked(mPglCore->option("updateAutomatically")->isEnabled());
    mUi.updateDailyRadio->setChecked(mPglCore->option("updateDailyRadio")->isEnabled());
    mUi.updateWeeklyRadio->setChecked(mPglCore->option("updateWeeklyRadio")->isEnabled());
    mUi.updateMonthlyRadio->setChecked(mPglCore->option("updateMonthlyRadio")->isEnabled());

    /*if ( PglSettings::value("INIT") == "0" )
        mUi.startAtBootCheckBox->setChecked(false);
    else if ( PglSettings::value("INIT") == "1" )
        mUi.startAtBootCheckBox->setChecked(true);


    if ( PglSettings::value("CRON") == "0" )
        mUi.updateAutomaticallyCheckBox->setChecked(false);
    else
    {
        QString frequency = mPglCore->getUpdateFrequencyCurrentPath();

        if ( ! frequency.isEmpty() )
        {
            mUi.updateAutomaticallyCheckBox->setChecked(true);
            if (frequency.contains("daily/", Qt::CaseInsensitive))
                mUi.updateDailyRadio->setChecked(true);
            else if ( frequency.contains("weekly/", Qt::CaseInsensitive))
                mUi.updateWeeklyRadio->setChecked(true);
            else if ( frequency.contains("monthly/", Qt::CaseInsensitive))
                mUi.updateMonthlyRadio->setChecked(true);
        }
    }*/

    updateBlocklistWidget();
    updateWhitelistWidget();
}

void Peerguardian::startTimers()
{
	//Intiallize the medium timer for less usual procedures
	m_MediumTimer = new QTimer(this);
	m_MediumTimer->setInterval( m_ProgramSettings->value("settings/medium_timer",MEDIUM_TIMER_INTERVAL ).toInt() );
	m_MediumTimer->start();
	//Intiallize the slow timer for even less usual procedures
	m_SlowTimer = new QTimer(this);
	m_SlowTimer->setInterval( m_ProgramSettings->value("settings/slow_timer", SLOW_TIMER_INTERVAL ).toInt() );
	m_SlowTimer->start();

}

void Peerguardian::g_MakeConnections()
{
	//Log tab connections
    connect( mUi.logTreeWidget, SIGNAL(customContextMenuRequested ( const QPoint &)), this, SLOT(showLogRightClickMenu(const QPoint &)));
    connect( mUi.logTreeWidget->verticalScrollBar(), SIGNAL(sliderMoved(int)), this, SLOT(onLogViewVerticalScrollbarMoved(int)));
    connect( mUi.logTreeWidget->verticalScrollBar(), SIGNAL(actionTriggered(int)), this, SLOT(onLogViewVerticalScrollbarActionTriggered(int)));
    connect( mUi.clearLogButton, SIGNAL( clicked() ), mUi.logTreeWidget, SLOT( clear() ) );
    connect(mUi.stopLoggingButton, SIGNAL(clicked()), this, SLOT(startStopLogging()));


    connect( mUi.addExceptionButton, SIGNAL(clicked()), this, SLOT(showAddExceptionDialog()) );
    connect( mUi.addBlockListButton, SIGNAL(clicked()), this, SLOT(showAddBlocklistDialog()) );

    //Menu related
    connect( mUi.a_Exit, SIGNAL( triggered() ), this, SLOT( quit() ) );
    connect( mUi.a_AboutDialog, SIGNAL( triggered() ), this, SLOT( g_ShowAboutDialog() ) );
    connect(mUi.viewPglcmdLogAction, SIGNAL(triggered()), this, SLOT(onViewerWidgetRequested()));
    connect(mUi.viewPgldLogAction, SIGNAL(triggered()), this, SLOT(onViewerWidgetRequested()));

    //Control related
    if ( m_Control ) {
        connect( mUi.startPglButton, SIGNAL( clicked() ), m_Control, SLOT( start() ) );
        connect( mUi.stopPglButton, SIGNAL( clicked() ), m_Control, SLOT( stop() ) );
        connect( mUi.restartPglButton, SIGNAL( clicked() ), m_Control, SLOT( restart() ) );
        connect( mUi.reloadPglButton, SIGNAL( clicked() ), m_Control, SLOT( reload() ) );
        connect( mUi.a_Start, SIGNAL( triggered() ), m_Control, SLOT( start() ) );
        connect( mUi.a_Stop, SIGNAL( triggered() ), m_Control, SLOT( stop() ) );
        connect( mUi.a_Restart, SIGNAL( triggered() ), m_Control, SLOT( restart() ) );
        connect( mUi.a_Reload, SIGNAL( triggered() ), m_Control, SLOT( reload() ) );
        connect( mUi.updatePglButton, SIGNAL( clicked() ), m_Control, SLOT( update() ) );
        connect( m_Control, SIGNAL(error(const QString&)), this, SLOT(rootError(const QString&)));
        connect( m_Control, SIGNAL(error(const CommandList&)), this, SLOT(rootError(const CommandList&)));
    }

    connect( m_MediumTimer, SIGNAL( timeout() ), this, SLOT( g_UpdateDaemonStatus() ) );
	connect( m_MediumTimer, SIGNAL( timeout() ), this, SLOT( updateInfo() ) );

    //status bar
    connect( m_Control, SIGNAL( actionMessage( QString, int ) ), mUi.statusBar, SLOT( showMessage( QString, int ) ) );
    connect( m_Control, SIGNAL( finished() ), mUi.statusBar, SLOT( clearMessage() ) );


	//Blocklist and Whitelist Tree Widgets
    connect(mUi.whitelistTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(whitelistItemChanged(QTreeWidgetItem*, int)));
    connect(mUi.blocklistTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(blocklistItemChanged(QTreeWidgetItem*,int)));
    connect(mUi.whitelistTreeWidget, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(treeItemPressed(QTreeWidgetItem*, int)));
    connect(mUi.blocklistTreeWidget, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(treeItemPressed(QTreeWidgetItem*, int)));

    /********************************Configure tab****************************/
    connect(mUi.undoButton, SIGNAL(clicked()), this, SLOT(undoAll()));
    connect(mUi.applyButton, SIGNAL(clicked()), this, SLOT(applyChanges()));
    connect(mUi.startAtBootCheckBox, SIGNAL(clicked(bool)), this, SLOT(checkboxChanged(bool)));
    connect(mUi.updateAutomaticallyCheckBox, SIGNAL(clicked(bool)), this, SLOT(checkboxChanged(bool)));

    //connect update frequency radio buttons
    connect(mUi.updateDailyRadio, SIGNAL(clicked(bool)), this, SLOT(updateRadioButtonToggled(bool)));
    connect(mUi.updateWeeklyRadio, SIGNAL(clicked(bool)), this, SLOT(updateRadioButtonToggled(bool)));
    connect(mUi.updateMonthlyRadio, SIGNAL(clicked(bool)), this, SLOT(updateRadioButtonToggled(bool)));

    if ( m_Root ) {
        connect(m_Root, SIGNAL(finished()), this, SLOT(rootFinished()));
        connect(m_Root, SIGNAL(error(const QString&)), this, SLOT(rootError(const QString&)));
        connect(m_Root, SIGNAL(error(const CommandList&)), this, SLOT(rootError(const CommandList&)));
    }
    
    //connect the remove buttons
    connect(mUi.removeBlocklistButton, SIGNAL(clicked()), this, SLOT(removeListItems()));
    connect(mUi.removeExceptionButton, SIGNAL(clicked()), this, SLOT(removeListItems()));
    
    //tray iconPath
    connect(m_Tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayIconClicked(QSystemTrayIcon::ActivationReason)));

    connect(mUi.a_SettingsDialog, SIGNAL(triggered()), this, SLOT(openSettingsDialog()));
}

void Peerguardian::quit()
{
    int answer;
    
    if (mUi.applyButton->isEnabled()) {
        answer = confirm(tr("Really quit?"), tr("You have <b>unapplied</b> changes, do you really want to quit?"), this);
        if ( answer == QMessageBox::No )
            return;
    }
    
    qApp->quit();
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
    QTreeWidget * tree;
    bool isWhitelist;

    if ( sender()->objectName().contains("block", Qt::CaseInsensitive) ) {
        tree  = mUi.blocklistTreeWidget;
        isWhitelist = false;
    }
    else {
        tree  = mUi.whitelistTreeWidget;
        isWhitelist = true;
    }

    foreach(QTreeWidgetItem *item, tree->selectedItems()) {
        QVariant _item = item->data(0, Qt::UserRole);

        if ( isWhitelist ) {
            WhitelistItem* whitelistItem = (WhitelistItem*) _item.value<void*>();
            whitelistItem->remove();
        }
        else {
            Blocklist* blocklist = (Blocklist*) _item.value<void*>();
            blocklist->remove();
        }
    }

    foreach(QTreeWidgetItem* item, tree->selectedItems())
        tree->takeTopLevelItem(tree->indexOfTopLevelItem(item));

    setApplyButtonEnabled(mPglCore->isChanged());
}

void Peerguardian::rootFinished()
{
    if ( m_FilesToMove.isEmpty() )
        return;
        
    WhitelistManager* whitelist = mPglCore->whitelistManager();
    QString pglcmd_conf = PglSettings::value("CMD_CONF").split("/").last();
    QString blocklists_list = PglSettings::value("BLOCKLISTS_LIST").split("/").last();
    QString tmp_pglcmd = QString("/tmp/%1").arg(pglcmd_conf);
    QString tmp_blocklists = QString("/tmp/%1").arg(blocklists_list);

    if ( ( m_FilesToMove.contains(tmp_pglcmd) && QFile::exists(tmp_pglcmd) ) ||  
        (m_FilesToMove.contains(tmp_blocklists) && QFile::exists(tmp_blocklists))){
        setApplyButtonEnabled(true);
    }
    else {
        whitelist->updateGuiSettings();
        mPglCore->load();
        updateGUI();
        setApplyButtonEnabled(false);
        
        if (mReloadPgl)
            m_Control->reload();
        mReloadPgl = false;
    }
    
    m_FilesToMove.clear();
}


void Peerguardian::rootError(const CommandList& failedCommands)
{
    ErrorDialog dialog(failedCommands);
    dialog.exec();

    
    /*QString errorMsg = QString("%1<br/><br/><i>%2</i><br/><br/>%3").arg(tr("The following commands failed:"))
                                                    .arg(commands.join("<br/>"))
                                                    .arg(tr("Please, check pgld's and/or pglcmd's log. You can do so through the <i>View menu</i>."));
    QMessageBox::warning( this, tr("Error (One or more command(s) failed)"), errorMsg,
	QMessageBox::Ok
    );*/
 
    setApplyButtonEnabled(mPglCore->isChanged());
}

void Peerguardian::rootError(const QString& errorMsg)
{
    QMessageBox::warning( this, tr("Error"), errorMsg,
	QMessageBox::Ok
    );
    
    setApplyButtonEnabled(mPglCore->isChanged());
}


void Peerguardian::updateRadioButtonToggled(bool toggled)
{
   QRadioButton * radioButton = qobject_cast<QRadioButton*>(sender());
   if (! radioButton)
       return;
     
    QList<QRadioButton*> radioButtons;
    radioButtons.append(mUi.updateDailyRadio);
    radioButtons.append(mUi.updateWeeklyRadio);
    radioButtons.append(mUi.updateMonthlyRadio);

    foreach(QRadioButton* button, radioButtons) {
        if (radioButton != button) {
            mPglCore->option(button->objectName())->setValue(false);
            setButtonChanged(button, false);
        }
    }

    Option* option = mPglCore->option(radioButton->objectName());
    if (option) {
        option->setValue(toggled);
        setButtonChanged(radioButton, option->isChanged());
        setApplyButtonEnabled(mPglCore->isChanged());
    }


    /*Option* updateFrequencyOption = mPglCore->option("blocklistsUpdateFrequency");
    if (updateFrequencyOption == item->text())

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

    if (item == mUi.updateDailyRadio) {

    }
    else if (item == mUi.updateWeeklyRadio) {
        mUi.updateDailyRadio->setIcon(QIcon());
        mUi.updateDailyRadio->setStatusTip("");
    }
    
    if ( item->objectName() != mUi.updateDailyRadio->objectName() )
    {
        mUi.updateDailyRadio->setIcon(QIcon());
        mUi.updateDailyRadio->setStatusTip("");
    }
    	
    if ( item->objectName() != mUi.updateWeeklyRadio->objectName() )
    {
        mUi.updateWeeklyRadio->setIcon(QIcon());
        mUi.updateWeeklyRadio->setStatusTip("");
    }
	
    if ( item->objectName() != mUi.updateMonthlyRadio->objectName() )
    {
        mUi.updateMonthlyRadio->setIcon(QIcon());
        mUi.updateMonthlyRadio->setStatusTip("");
    }*/
}


void Peerguardian::checkboxChanged(bool state)
{
    QCheckBox *button = qobject_cast<QCheckBox*>(sender());
    if (! button)
        return;

    QString senderName = sender()->objectName();
    bool changed = false;

    if (senderName.contains("startAtBoot")) {
        mPglCore->setOption("startAtBoot", state);
        changed = mPglCore->option("startAtBoot")->isChanged();
    }
    else if (senderName.contains("updateAutomatically")){
        mPglCore->setOption("updateAutomatically", state);
        changed = mPglCore->option("updateAutomatically")->isChanged();
    }
    else
        return;

    setButtonChanged(button, changed);
    /*if  (changed) {
        item->setIcon(QIcon(WARNING_ICON));
        item->setStatusTip(tr("You need to click the Apply button so the changes take effect"));
    }
    else {
        item->setIcon(QIcon());
        item->setStatusTip("");
    }*/

    setApplyButtonEnabled(mPglCore->isChanged());

    /*mUi.applyButton->setEnabled(guiOptions->isChanged());
    mUi.undoButton->setEnabled(mUi.applyButton->isEnabled());
    
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
    }*/
}



QString Peerguardian::getUpdateFrequencyPath()
{
    QString path("/etc/cron.");
    QString script ("pglcmd");

    if ( mUi.updateDailyRadio->isChecked() )
        return path += "daily/" + script;
    else if ( mUi.updateWeeklyRadio->isChecked() )
        return path += "weekly/" + script;
    else if ( mUi.updateMonthlyRadio->isChecked() )
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
    mReloadPgl = false;
    WhitelistManager* whitelist = mPglCore->whitelistManager();
    BlocklistManager* blocklistManager = mPglCore->blocklistManager();
    QMap<QString, QString> filesToMove;
    QStringList pglcmdConf;
    QString pglcmdConfPath = PglSettings::value("CMD_CONF");
    bool updatePglcmdConf = mPglCore->hasToUpdatePglcmdConf();
    bool updateBlocklistsFile = mPglCore->hasToUpdateBlocklistsFile();
    QString filepath;
    
    if ( pglcmdConfPath.isEmpty() ) {
        QString errorMsg = tr("Could not determine pglcmd.conf path! Did you install pgld and pglcmd?");
        QMessageBox::warning( this, tr("Error"), errorMsg, QMessageBox::Ok);
        qWarning() << errorMsg;
        return;
    }
    
    //only apply IPtables commands if the daemon is running
    if ( m_Info->daemonState() )
    {
        //apply new changes directly in iptables
        // = whitelist->updateWhitelistItemsInIptables(getTreeItems(mUi.whitelistTreeWidget), guiOptions);
        QStringList iptablesCommands = whitelist->generateIptablesCommands();
        if ( ! iptablesCommands.isEmpty() )
            m_Root->executeCommands(iptablesCommands, false);
    }
    
    //================ update /etc/pgl/pglcmd.conf ================/
    if ( updatePglcmdConf ) {
        //======== Whitelisted IPs are stored in pglcmd.conf too ==========/
        //pglcmdConf = whitelist->updatePglcmdConf(getTreeItems(mUi.whitelistTreeWidget));
        pglcmdConf = mPglCore->generatePglcmdConf();

        //========start at boot option ( pglcmd.conf )==========/
        /*QString value (QString::number(int(mUi.startAtBootCheckBox->isChecked())));
        if ( hasValueInData("INIT", pglcmdConf) || PglSettings::value("INIT") != value )
            pglcmdConf = replaceValueInData(pglcmdConf, "INIT", value);

        //====== update  frequency check box ( pglcmd.conf ) ==========/
        value = QString::number(int(mUi.updateAutomaticallyCheckBox->isChecked()));
        if ( hasValueInData("CRON", pglcmdConf) || PglSettings::value("CRON") != value )
            pglcmdConf = replaceValueInData(pglcmdConf, "CRON", value);*/

        //add /tmp/pglcmd.conf to the filesToMove
        QString  pglcmdTempPath = QDir::temp().absoluteFilePath(getFileName(pglcmdConfPath));
        filesToMove[pglcmdTempPath] = pglcmdConfPath;
        saveFileData(pglcmdConf, pglcmdTempPath);
    }

    //================ update /etc/pgl/blocklists.list ================/
    if ( updateBlocklistsFile ) {

        mReloadPgl = true;
        QStringList data = blocklistManager->generateBlocklistsFile();
        QString outputFilePath = QDir::temp().absoluteFilePath(getFileName(blocklistManager->blocklistsFilePath()));
        saveFileData(data, outputFilePath);
        
        //update the blocklists.list file
        if (QFile::exists(outputFilePath))
            filesToMove[outputFilePath] = blocklistManager->blocklistsFilePath();

        //================ manage the local blocklists ====================/
        QDir localBlocklistDir(blocklistManager->localBlocklistsDir());
        QList<Blocklist*> localBlocklists = blocklistManager->localBlocklists();
        QDir tempDir = QDir::temp();
        
        foreach(Blocklist* blocklist, localBlocklists) {
            if (! blocklist->isChanged() || ! blocklist->exists())
                continue;

            filepath = blocklist->location();
            if (blocklist->isAdded()) {
                QFile::link(filepath, tempDir.absoluteFilePath(blocklist->name()));
                filesToMove[tempDir.absoluteFilePath(blocklist->name())] = localBlocklistDir.absolutePath();
            }
            else if (blocklist->isRemoved()) {
                filesToMove[blocklist->location()] = "/dev/null";
            }
            else if (blocklist->isEnabled()) {
               filesToMove[blocklist->location()] = localBlocklistDir.absoluteFilePath(blocklist->name());
            }
            else if (blocklist->isDisabled()) {
                filesToMove[blocklist->location()] = localBlocklistDir.absoluteFilePath("."+blocklist->name());
            }
        }
        
        /*foreach(const QString& name, localFiles.keys())
        {
            QFileInfo info(filepath);
            if (! info.exists())
                continue;
            
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
        }*/
    }


    //====== update  frequency radio buttons ==========/
    filepath = getUpdateFrequencyPath();
    if ( ! QFile::exists(filepath) )
        filesToMove[getUpdateFrequencyCurrentPath()] = filepath;
        
    m_FilesToMove = filesToMove.keys();

    if ( ! filesToMove.isEmpty() )
        m_Root->moveFiles(filesToMove, false);

    //whitelist->updateSettings(getTreeItems(mUi.whitelistTreeWidget), guiOptions->getPositionFirstAddedWhitelistItem(), false);
    //guiOptions->updateWhitelist(guiOptions->getPositionFirstAddedWhitelistItem(), false);

    //assume changes will be applied, if not this button will be enabled afterwards
    setApplyButtonEnabled(false);

    m_Root->executeScript(); //execute previous gathered commands
}


QList<QTreeWidgetItem*> Peerguardian::getTreeItems(QTreeWidget *tree, int checkState)
{
	QList<QTreeWidgetItem*> items;

	for (int i=0; i < tree->topLevelItemCount(); i++ )
		if ( tree->topLevelItem(i)->checkState(0) == checkState || checkState == -1)
			items << tree->topLevelItem(i);

	return items;
}

void Peerguardian::blocklistItemChanged(QTreeWidgetItem* item, int column)
{
    QTreeWidget* treeWidget = item->treeWidget();
    if (! treeWidget)
        return;

    BlocklistManager* manager = mPglCore->blocklistManager();
    //Blocklist* blocklist = manager->blocklistAt(treeWidget->indexOfTopLevelItem(item));
    QVariant bl = item->data(0, Qt::UserRole);
    Blocklist* blocklist = (Blocklist*) bl.value<void*>();
    if (blocklist) {
        if (item->checkState(0) == Qt::Checked)
            blocklist->setEnabled(true);
        else
            blocklist->setEnabled(false);

        setTreeWidgetItemChanged(item, blocklist->isChanged());
        setApplyButtonEnabled(mPglCore->isChanged());
    }
}

void Peerguardian::whitelistItemChanged(QTreeWidgetItem* item, int column)
{
    QTreeWidget* treeWidget = item->treeWidget();
    if (! treeWidget)
        return;

    WhitelistManager* manager = mPglCore->whitelistManager();
    //WhitelistItem* whitelistItem = manager->whitelistItemAt(treeWidget->indexOfTopLevelItem(item));
    QVariant wlitem = item->data(0, Qt::UserRole);
    WhitelistItem* whitelistItem = (WhitelistItem*) wlitem.value<void*>();

    if (whitelistItem) {
        if (item->checkState(0) == Qt::Checked)
            whitelistItem->setEnabled(true);
        else
            whitelistItem->setEnabled(false);

        setTreeWidgetItemChanged(item, whitelistItem->isChanged());
        setApplyButtonEnabled(mPglCore->isChanged());
    }
}

void Peerguardian::treeItemChanged(QTreeWidgetItem* item, int column)
{
    //mUi.whitelistTreeWidget->blockSignals(true);
    //mUi.blocklistTreeWidget->blockSignals(true);
    /*QTreeWidget* treeWidget = item->treeWidget();
    if (! treeWidget)
        return;

    treeWidget->blockSignals(true);

    //bool changed = guiOptions->isChanged();
    bool changed = mPglCore->isChanged();
    mUi.applyButton->setEnabled(changed);
    mUi.undoButton->setEnabled(changed);
    
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
    
    treeWidget->blockSignals(false);
    //mUi.whitelistTreeWidget->blockSignals(false);
    //mUi.blocklistTreeWidget->blockSignals(false);*/
}

void Peerguardian::treeItemPressed(QTreeWidgetItem* item, int column)
{
    if ( item->treeWidget()->objectName().contains("block", Qt::CaseInsensitive) )
    {
        mUi.removeExceptionButton->setEnabled(false);
        mUi.removeBlocklistButton->setEnabled(true);
    }
    else
    {
        mUi.removeExceptionButton->setEnabled(true);
        mUi.removeBlocklistButton->setEnabled(false);
    }

}

void Peerguardian::updateBlocklistWidget()
{

    //m_List->loadBlocklists();
    BlocklistManager* blocklistManager = mPglCore->blocklistManager();
    //blocklistManager->loadBlocklists();

    mUi.blocklistTreeWidget->blockSignals(true);
    if (mUi.blocklistTreeWidget->topLevelItemCount())
        mUi.blocklistTreeWidget->clear();

    QStringList info;

    //get information about the blocklists being used
    foreach(Blocklist* blocklist, blocklistManager->blocklists()) {
        info << blocklist->name();
        QTreeWidgetItem * item = new QTreeWidgetItem(mUi.blocklistTreeWidget, info);
        item->setToolTip(0, blocklist->targetLocation());
        item->setData(0, Qt::UserRole, qVariantFromValue((void *) blocklist));
        /*QVariant bl = item->data(0, Qt::UserRole);
        Blocklist *block = (Blocklist*) bl.value<void*>();
        qDebug() << block << blocklist;*/


        if ( blocklist->isEnabled() )
            item->setCheckState(0, Qt::Checked);
        else
            item->setCheckState(0, Qt::Unchecked);
        setTreeWidgetItemChanged(item, blocklist->isChanged(), false);

        info.clear();
    }

    mUi.blocklistTreeWidget->blockSignals(false);
}

void Peerguardian::updateWhitelistWidget()
{
    QStringList info;
    WhitelistManager *whitelist = mPglCore->whitelistManager();
    
    mUi.whitelistTreeWidget->blockSignals(true);
    
    if ( mUi.whitelistTreeWidget->topLevelItemCount() > 0 )
        mUi.whitelistTreeWidget->clear();
    
    foreach(WhitelistItem * item, whitelist->whitelistItems()) {
        info << item->value() << item->connection() << item->protocol();
        QTreeWidgetItem * treeItem = new QTreeWidgetItem(mUi.whitelistTreeWidget, info);
        treeItem->setData(0, Qt::UserRole, qVariantFromValue((void *) item));

        if (item->isEnabled())
            treeItem->setCheckState(0, Qt::Checked );
        else
            treeItem->setCheckState(0, Qt::Unchecked );

        setTreeWidgetItemChanged(treeItem, item->isChanged(), false);
        info.clear();
    }

    mUi.whitelistTreeWidget->setSortingEnabled(true);
    mUi.whitelistTreeWidget->sortByColumn(0);
    mUi.whitelistTreeWidget->blockSignals(false);


    /*mUi.whitelistTreeWidget->blockSignals(true);

    //get enabled whitelisted IPs and ports
    items = m_Whitelist->getEnabledWhitelistedItems();
    foreach(QString key, items.keys())
    {
        values = items[key];
        foreach( QString value, values )
        {
            info << value << m_Whitelist->getTypeAsString(key) << m_Whitelist->parseProtocol(key);
            QTreeWidgetItem * tree_item = new QTreeWidgetItem(mUi.whitelistTreeWidget, info);
            tree_item->setCheckState(0, Qt::Checked );
            mUi.whitelistTreeWidget->addTopLevelItem(tree_item);
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
            info << value << m_Whitelist->getTypeAsString(key) << m_Whitelist->parseProtocol(key);
            QTreeWidgetItem * tree_item = new QTreeWidgetItem(mUi.whitelistTreeWidget, info);
            tree_item->setCheckState(0, Qt::Unchecked );
            mUi.whitelistTreeWidget->addTopLevelItem(tree_item);
            info.clear();
        }
    }
    
    mUi.whitelistTreeWidget->setSortingEnabled(true);
    
    mUi.whitelistTreeWidget->blockSignals(false);*/
}

void Peerguardian::initCore()
{
	//Intiallize all pointers to NULL before creating the objects with g_Set==Path
    m_Info = 0;
    m_Root = 0;
    m_Control = 0;
    quitApp = false;
    mLastRunningState = false;

    mPglCore = new PglCore();
    mPglCore->load();
    
    m_ProgramSettings = new QSettings(QSettings::UserScope, "pgl", "pglgui", this);
    
    g_SetRoot();
    g_SetInfoPath();
    g_SetControlPath();
}

void Peerguardian::g_SetRoot( ) {
    
    if ( m_Root )
        delete m_Root;
    
    m_Root = new SuperUser(this, m_ProgramSettings->value("paths/super_user", "").toString());
}

void Peerguardian::g_SetInfoPath() {
    if (! m_Info)
        m_Info = new PeerguardianInfo(PglSettings::value("DAEMON_LOG"), this);
}

void Peerguardian::g_SetControlPath()
{
    QString  gSudo = m_ProgramSettings->value("paths/super_user").toString();
    m_Control = new PglCmd(this, PglSettings::value("CMD_PATHNAME"), gSudo);
    
}

void Peerguardian::g_ShowAddDialog(int openmode) {
    AddExceptionDialog *dialog = 0;
    bool newItems = false;

    if ( openmode == (ADD_MODE | EXCEPTION_MODE) )
    {
        dialog = new AddExceptionDialog( this, openmode, mPglCore);
        dialog->exec();

        WhitelistManager* whitelist = mPglCore->whitelistManager();

        foreach(WhitelistItem whiteItem, dialog->getItems()) {
            QStringList info; info << whiteItem.value() << whiteItem.connection() << whiteItem.protocol();
            whitelist->addItem(new WhitelistItem(whiteItem.value(), whiteItem.connection(), whiteItem.protocol()));
            //QTreeWidgetItem * treeItem = new QTreeWidgetItem(mUi.whitelistTreeWidget, info);
            //treeItem->setCheckState(0, Qt::Checked);
            //treeItem->setIcon(0, QIcon(WARNING_ICON));
            //treeItem->setStatusTip(0, tr("You need to click the Apply button so the changes take effect"));
            //mUi.whitelistTreeWidget->addTopLevelItem(treeItem);

            newItems = true;
        }

        updateWhitelistWidget();
        mUi.whitelistTreeWidget->scrollToBottom();

	}
    else if (  openmode == (ADD_MODE | BLOCKLIST_MODE) )
    {

        dialog = new AddExceptionDialog( this, openmode, mPglCore);
        dialog->exec();
        BlocklistManager* blocklistManager = mPglCore->blocklistManager();

        foreach(const QString& blocklist, dialog->getBlocklists()) {
            qDebug() << blocklist;
            blocklistManager->addBlocklist(blocklist);
            newItems = true;
        }
        
        updateBlocklistWidget();
        mUi.blocklistTreeWidget->scrollToBottom();
    }

    if ( newItems ) {
        setApplyButtonEnabled(true);
    }



	/*if ( dialog->exec() == QDialog::Accepted && dialog->isSettingChanged() ) {
		emit g_SettingChanged();
	}
	else {
		g_ReloadSettings();
		m_SettingChanged = false;
	}*/

    if ( dialog )
        delete dialog;
}

void Peerguardian::updateBlocklists()
{
    QList<Blocklist*> blocklists = mPglCore->blocklistManager()->blocklists();
    
    /*foreach(Blocklist* blocklist, blocklists) {
        QStringList info; 
        info << name <<  blocklist;
        QTreeWidgetItem * treeItem = new QTreeWidgetItem(mUi.blocklistTreeWidget, info);
        treeItem->setCheckState(0, Qt::Checked);
        treeItem->setIcon(0, QIcon(WARNING_ICON));
        treeItem->setStatusTip(0, tr("You need to click the Apply button so the changes take effect"));
        newItems = true;
    }*/
}


void Peerguardian::g_MakeTray() 
{
	m_Tray = new QSystemTrayIcon( mTrayIconDisabled );
	m_Tray->setVisible( true );
    m_Tray->setToolTip(tr("Pgld is not running"));
	g_UpdateDaemonStatus();
}


void Peerguardian::g_UpdateDaemonStatus() {

    if (! m_Info )
        return;
    
	m_Info->updateDaemonState();
	bool running = m_Info->daemonState();
    
    if ( mLastRunningState != running ) {
        if ( ! running ) {
            mUi.controlPglButtons->setCurrentIndex(0);
            m_Tray->setIcon(mTrayIconDisabled);
            setWindowIcon(mTrayIconDisabled);
            m_Tray->setToolTip(tr("Pgld is not running"));
        }
        else {
            mUi.controlPglButtons->setCurrentIndex(1);
            m_Tray->setIcon(mTrayIconEnabled);
            setWindowIcon(mTrayIconEnabled);
            m_Tray->setToolTip(tr("Pgld is up and running"));
        }
    }

    mLastRunningState = running;
}

void Peerguardian::g_MakeMenus() {


    //tray icon menu
	m_TrayMenu = new QMenu(this);
    m_TrayMenu->addAction( mUi.a_Start );
    m_TrayMenu->addAction( mUi.a_Stop );
    m_TrayMenu->addAction( mUi.a_Restart );
    m_TrayMenu->addAction( mUi.a_Reload );
	/*m_TrayMenu->addSeparator();
	m_TrayMenu->addAction( a_AllowHttp );
	m_TrayMenu->addAction( a_AllowHttps );
	m_TrayMenu->addAction( a_AllowFtp );*/
	m_TrayMenu->addSeparator();
    m_TrayMenu->addAction( mUi.a_Exit );
	m_Tray->setContextMenu(m_TrayMenu);
    

}

void Peerguardian::g_ShowAboutDialog() 
{

    QString message;
    message += QString("<b><i>PeerGuardian Linux version %1</b><br>A Graphical User Interface for PeerGuardian Linux<br><br>").arg( VERSION_NUMBER );
    message += "Copyright (C) 2007-2008 Dimitris Palyvos-Giannas<br>";
    message += "Copyright (C) 2011-2013 Carlos Pais <br><br>";
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

void Peerguardian::updateInfo()
{
    if ( ! m_Info )
        return;

	QString lRanges = m_Info->loadedRanges();
	QString dTime = m_Info->lastUpdateTime();

	if ( lRanges.isEmpty() )
		lRanges = "N/A";

	if ( m_Info->daemonState() == false )
		lRanges = "";
    else
        lRanges.insert(0, " - ");

	//if ( dTime.isNull() )
	//	dTime = "Unknown";
        
    if ( windowTitle() != DEFAULT_WINDOW_TITLE + lRanges )
        setWindowTitle(DEFAULT_WINDOW_TITLE + lRanges);
}


void Peerguardian::undoAll()
{ 
    int answer = 0;
    
    answer = confirm(tr("Really Undo?"), tr("Are you sure you want to ignore the unsaved changes?"), this);
    
    if ( answer == QMessageBox::Yes) {
        mPglCore->undo();
        updateGUI();
        setApplyButtonEnabled(false);
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
    SettingsDialog * dialog = new SettingsDialog(m_ProgramSettings, this);
    
    int exitCode = dialog->exec();
    
    if ( exitCode )
    {
        m_ProgramSettings->setValue("paths/super_user", dialog->file_GetRootPath());
        SuperUser::setSudoCommand(m_ProgramSettings->value("paths/super_user", SuperUser::sudoCommand()).toString());
        m_MaxLogSize = dialog->getMaxLogEntries();
        m_ProgramSettings->setValue("maximum_log_entries", QString::number(m_MaxLogSize));
    }
    
    if ( dialog )
        delete dialog; 
}


void Peerguardian::showLogRightClickMenu(const QPoint& p)
{
    QTreeWidgetItem * item = mUi.logTreeWidget->itemAt(p);
    
    if ( ! item )
        return;
    
    QMenu menu(this);
    QMenu *menuIp;
    QMenu *menuPort;
    int index = 4;
    
    if ( item->text(7) == "Incoming" )
        index = 2;
    
    QVariantMap data;
    data.insert("ip", item->text(index));
    data.insert("port", item->text(5));
    data.insert("prot", item->text(6));
    data.insert("type", item->text(7));
    
    a_whitelistIpTemp->setData(data);
    a_whitelistIpPerm->setData(data);
    a_whitelistPortTemp->setData(data);
    a_whitelistPortPerm->setData(data);
    
    menuIp =  menu.addMenu("Allow IP " + item->text(index));
    menuPort = menu.addMenu("Allow Port " + item->text(5));
    
    menu.addSeparator();
    aWhoisIp->setData(item->text(index));
    aWhoisIp->setText(tr("Whois ") + item->text(index));
    menu.addAction(aWhoisIp);
    
    menuIp->addAction(a_whitelistIpTemp);
    menuIp->addAction(a_whitelistIpPerm);
    menuPort->addAction(a_whitelistPortTemp);
    menuPort->addAction(a_whitelistPortPerm);

    menu.exec(mUi.logTreeWidget->mapToGlobal(p));
}

void Peerguardian::whitelistItem()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (! action)
        return;
    
    if (! m_Info->daemonState()) {
        QMessageBox::information(this, tr("Peerguardian is not running"), tr("It's not possible to whitelist while Peerguardian is not running."));
        return;
    }
    
    WhitelistManager* whitelist = mPglCore->whitelistManager();
    QVariantMap data = action->data().toMap();
    QString ip = data.value("ip").toString();
    QString port = data.value("port").toString();
    QString type = data.value("type").toString();
    QString prot = data.value("prot").toString();
    QString value = "";
    if (action == a_whitelistIpPerm || action == a_whitelistIpTemp)
        value = ip;
    else
        value = port;
        
    if ( action == a_whitelistIpTemp || action ==  a_whitelistPortTemp )
    {
        QStringList iptablesCommands = whitelist->getCommands(QStringList() << value, QStringList() << type, QStringList() << prot, QList<bool>() << true);
        QString testCommand = whitelist->getIptablesTestCommand(ip, type, prot);
        m_Root->executeCommands(iptablesCommands, false);
        m_Root->executeScript();
    }
    else if (  action == a_whitelistIpPerm || action == a_whitelistPortPerm )
    {
        if ( ! whitelist->contains(value, type, prot) )
        {
            QStringList info;
            info << value << type << prot;
            QTreeWidgetItem * treeItem = new QTreeWidgetItem(mUi.whitelistTreeWidget, info);
            treeItem->setCheckState(0, Qt::Checked);
            treeItem->setIcon(0, QIcon(WARNING_ICON));
            treeItem->setStatusTip(0, tr("You need to click the Apply button so the changes take effect"));
            mUi.whitelistTreeWidget->addTopLevelItem(treeItem);
            applyChanges();
        }
    }
}

void Peerguardian::onViewerWidgetRequested()
{
    QString path("");

    if ( mUi.viewPglcmdLogAction == sender() ) {
        path = PglSettings::value("CMD_LOG");
    }
    else if (mUi.viewPgldLogAction == sender()) {
        path = PglSettings::value("DAEMON_LOG");
    }
    
    ViewerWidget viewer(path);
    viewer.exec();
}

bool Peerguardian::eventFilter(QObject* obj, QEvent* event)
{
    //if (obj == mUi.logTreeWidget->verticalScrollBar() && mIgnoreScroll)
    //    return true;
    
    if (obj == mUi.logTreeWidget->verticalScrollBar() && event->type() == QEvent::Wheel) {
        
        if (mUi.logTreeWidget->verticalScrollBar()->value() == mUi.logTreeWidget->verticalScrollBar()->maximum())
            mAutomaticScroll = true;
        else
            mAutomaticScroll = false;
    }
    
    return false;
}

void Peerguardian::onLogViewVerticalScrollbarMoved(int value)
{
    QScrollBar *bar = static_cast<QScrollBar*>(sender());

    if (bar->maximum() == value) 
        mAutomaticScroll = true;
    else
        mAutomaticScroll = false;
}

void Peerguardian::onLogViewVerticalScrollbarActionTriggered(int action) 
{
    QScrollBar *scrollBar = static_cast<QScrollBar*>(sender());
    
    if (mAutomaticScroll && scrollBar->value() < scrollBar->maximum())
        scrollBar->setSliderPosition(scrollBar->maximum());
        
}

void Peerguardian::showCommandsOutput(const CommandList& commands) 
{
    
    QString output("");
    QString title("");
    foreach(const Command& command, commands) {
        output += command.output();
        output += "\n";
        
        if (! title.isEmpty())
            title += tr(" and ");
        title += command.command();
    }
    
    ViewerWidget viewer(output);
    viewer.setWindowTitle(title);
    viewer.exec();
} 

void Peerguardian::onWhoisTriggered() 
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (! action)
        return;
    ProcessT *process = new ProcessT(this);
    connect(process, SIGNAL(finished(const CommandList&)), this, SLOT(showCommandsOutput(const CommandList&)));
    if (action->text().contains(" "))
        process->execute("whois", QStringList() << action->data().toString());
}

void Peerguardian::showAddBlocklistDialog()
{
    g_ShowAddDialog(ADD_MODE | BLOCKLIST_MODE);
}

void Peerguardian::showAddExceptionDialog()
{
    g_ShowAddDialog(ADD_MODE | EXCEPTION_MODE);
}

void Peerguardian::setApplyButtonEnabled(bool enable)
{
    mUi.applyButton->setEnabled(enable);
    mUi.undoButton->setEnabled(enable);
}

void Peerguardian::setButtonChanged(QAbstractButton* button, bool changed)
{
    if (changed) {
        button->setIcon(QIcon(WARNING_ICON));
        button->setStatusTip(tr("You need to click the Apply button so the changes take effect"));
    }
    else {
        button->setIcon(QIcon());
        button->setStatusTip("");
    }
}

void Peerguardian::setTreeWidgetItemChanged(QTreeWidgetItem* item, bool changed, bool blockSignals)
{
    QTreeWidget* treeWidget = item->treeWidget();
    if (! treeWidget)
        return;

    if (blockSignals)
        treeWidget->blockSignals(true);

    if (changed) {
        item->setIcon(0, QIcon(WARNING_ICON));
        item->setStatusTip(0, tr("You need to click the Apply button so the changes take effect"));
    }
    else {
        item->setIcon(0, QIcon());
        item->setStatusTip(0, "");
    }

    if (blockSignals)
        treeWidget->blockSignals(false);
}
