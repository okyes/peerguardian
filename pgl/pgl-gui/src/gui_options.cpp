#include "gui_options.h"

GuiOptions::GuiOptions() 
{ 
    m_Window = NULL; 
    m_WhitelistItemsRemoved = 0;
}

GuiOptions::GuiOptions(Peerguardian * gui)
{ 
    m_Window = gui; 
    m_WhitelistItemsRemoved = 0;
}

GuiOptions::~GuiOptions(){ 
    
    deleteItems(m_Whitelist);
    deleteItems(m_Blocklist);
    qDebug() << "~GuiOptions()"; 
    
}

bool GuiOptions::isChanged()
{

    if (startAtBoot != m_Window->getStartAtBootBox()->isChecked() )
        return true;

    if (updateBlocklistsAutomatically != m_Window->getAutoListUpdateBox()->isChecked() )
        return true;

    if ( updateRadioBtnName != getActiveUpdateRadioButtonName() )
        return true;

    if ( blocklistStateChanged() )
        return true;

    if ( whitelistStateChanged() )
        return true;

    return false;
}

bool GuiOptions::hasToUpdatePglcmdConf()
{
    if (startAtBoot != m_Window->getStartAtBootBox()->isChecked() )
        return true;

    if (updateBlocklistsAutomatically != m_Window->getAutoListUpdateBox()->isChecked() )
        return true;

    if ( whitelistStateChanged(false) )
        return true;

    return false;
}

bool GuiOptions::hasToUpdateBlocklistList()
{
    return blocklistStateChanged();
}

bool GuiOptions::whitelistStateChanged(bool anyChange)
{
    QTreeWidget *tree = m_Window->getWhitelistTreeWidget();
    
    if ( anyChange )
    {
        if ( m_WhitelistItemsRemoved )
            return true;
        
        if ( tree->topLevelItemCount() != m_WhitelistState.size() )
            return true;
    }
    else //Used to check if pglcmd.conf needs to be updated
    {
        //if these items need to be removed from iptables, they also need to be removed
        //from pglcmd.conf
        if ( ! m_WhitelistItemsForIptablesRemoval.isEmpty() )
            return true;
        
        //Check if there were added items
        int firstPos = getPositionFirstAddedWhitelistItem();
        if ( firstPos > 0 )
        {
            for(int i = firstPos; i < tree->topLevelItemCount(); i++)
                //Since this is to test if we need to update pglcmd.conf
                //it only interests us the ones that are checked 
                if ( tree->topLevelItem(i)->checkState(0) == Qt::Checked )  
                    return true;
        }
    }
    

    for ( int i=0; i < tree->topLevelItemCount() && i < m_WhitelistState.size(); i++ )
    {
        QTreeWidgetItem * item = tree->topLevelItem(i);

        if ( item->checkState(0) != m_WhitelistState[i] )
            return true;
    }

    return false;
}

bool GuiOptions::blocklistStateChanged()
{
    
    QTreeWidget *tree = m_Window->getBlocklistTreeWidget();

    if ( tree->topLevelItemCount() != m_BlocklistState.size() )
            return true;
    
    for ( int i=0; i < tree->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem * item = tree->topLevelItem(i);

        if ( item->checkState(0) != m_BlocklistState[i] )
            return true;
    }

    return false;
}


QRadioButton * GuiOptions::getActiveUpdateRadioButton()
{
    QRadioButton * radio = NULL;

    if ( m_Window->getAutoListUpdateDailyRadio()->isChecked() )
        radio = m_Window->getAutoListUpdateDailyRadio();
    else if ( m_Window->getAutoListUpdateWeeklyRadio()->isChecked() )
        radio = m_Window->getAutoListUpdateWeeklyRadio();
    else if ( m_Window->getAutoListUpdateMonthlyRadio()->isChecked() )
        radio = m_Window->getAutoListUpdateMonthlyRadio();
        
    return radio;
}

QString GuiOptions::getActiveUpdateRadioButtonName()
{
    QRadioButton * radio = getActiveUpdateRadioButton();
    if ( radio != NULL ) 
        return radio->objectName();
    return QString("");
}

int GuiOptions::getPositionFirstAddedWhitelistItem()
{
    int nItems = m_Window->getWhitelistTreeWidget()->topLevelItemCount();
    int prevNItems = m_WhitelistState.size();
    int added = nItems - ( prevNItems - m_WhitelistItemsRemoved );
    int pos = nItems - added;
    
    return pos;
}

void GuiOptions::addRemovedWhitelistItem(QTreeWidgetItem * item)
{
    if (  item->checkState(0) == Qt::Checked && item->icon(0).isNull() )
    {
        m_WhitelistItemsForIptablesRemoval << item;
        item->setIcon(0, QIcon(WARNING_ICON));
    }
       
    m_WhitelistItemsRemoved++;
}


/********* Update methods ************/

void GuiOptions::updateUpdateRadioBtn()
{
    updateBlocklistsAutomatically = m_Window->getAutoListUpdateBox()->isChecked();
    updateRadioBtnName = getActiveUpdateRadioButtonName();
    updateRadioBtn = getActiveUpdateRadioButton();
}


void GuiOptions::updateStartAtBoot()
{
    startAtBoot = m_Window->getStartAtBootBox()->isChecked();
}

void GuiOptions::deleteItems(QList<QTreeWidgetItem*> & list)
{
    foreach(QTreeWidgetItem * item, list)
        if (item) delete item;
    list.clear();
}

void GuiOptions::updateWhitelist(int startFrom)
{
    QTreeWidgetItem * item;
    QTreeWidget * tree = m_Window->getWhitelistTreeWidget();
    
    if ( startFrom <= 0)
    {
        startFrom = 0;
        deleteItems(m_Whitelist);
    }

    for ( int i=startFrom; i < tree->topLevelItemCount(); i++ )
    {
        item = tree->topLevelItem(i);
        QTreeWidgetItem * item2 = new QTreeWidgetItem(*item);
        m_WhitelistState << item->checkState(0);
        m_Whitelist << item2;
    }
    
}

void GuiOptions::updateBlocklist()
{
    QTreeWidgetItem * item;
    QTreeWidget * tree = m_Window->getBlocklistTreeWidget();

    deleteItems(m_Blocklist);

    for ( int i=0; i < tree->topLevelItemCount(); i++ )
    {
        item = tree->topLevelItem(i);
        QTreeWidgetItem * item2 = new QTreeWidgetItem(*item);
        m_Blocklist << item2;
        m_BlocklistState << item->checkState(0);
    }
    
    
}

void GuiOptions::update()
{
    m_WhitelistItemsRemoved = 0;
    m_WhitelistItemsForIptablesRemoval.clear();
    updateStartAtBoot();
    updateUpdateRadioBtn();
    updateWhitelist();
    updateBlocklist();
}


bool GuiOptions::isChanged(QTreeWidgetItem * item)
{
    QTreeWidget * treeWidget = item->treeWidget();
    int index = treeWidget->indexOfTopLevelItem(item);
    QList<int> listState;

    //this shouldn't happen, but just in case
    if ( index == -1 )
        return false;

    if ( treeWidget->objectName().contains("block", Qt::CaseInsensitive) )
        listState =  m_BlocklistState;
    else
        listState =  m_WhitelistState;

    //if the item has not yet been added to the listState, it means it's a new item
    if ( index > listState.size() - 1 )
        return true;

    //if the item's state is different from the original state, it's changed
    if ( item->checkState(0) != listState[index] )
        return true;

    return false;

}

bool GuiOptions::hasCheckboxChanged(QCheckBox * checkbox)
{
    if ( checkbox == m_Window->getStartAtBootBox()  && startAtBoot != m_Window->getStartAtBootBox()->isChecked())
	return true;
    else if ( checkbox == m_Window->getAutoListUpdateBox() && updateBlocklistsAutomatically != m_Window->getAutoListUpdateBox()->isChecked() )
        return true;
    
    return false;
}

bool GuiOptions::hasRadioButtonChanged(QRadioButton * radio)
{
    if ( radio->objectName() !=  updateRadioBtnName )
        return true;
    
    return false;
}

Qt::CheckState GuiOptions::getState(int state)
{
    
    switch(state)
    {
        case 0: return Qt::Unchecked;
        break;
        
        case 1: return Qt::PartiallyChecked;
        break;
        
        case 2: return Qt::Checked;
        break;
    }
}

/*void GuiOptions::undoBlocklist()
{
    //reset the blocklist and whitelist treewidgets
    QTreeWidget * tree = m_Window->getBlocklistTreeWidget();
    QTreeWidgetItem * item = NULL;
    
    for(int i = 0; i < m_BlocklistState.size() ; i++)
    {
        item = tree->topLevelItem(i);
        item->setCheckState(0, getState(m_BlocklistState[i]));
        item->setIcon(0, QIcon());
    }
    
    //remove added blocklists
    for(int i = m_BlocklistState.size(); i < tree->topLevelItemCount() ; i++)
    {
        tree->takeTopLevelItem(i);
    }
}*/

/*void GuiOptions::undoWhitelist()
{
    //reset the blocklist and whitelist treewidgets
    QTreeWidget * tree = m_Window->getWhitelistTreeWidget();
    QTreeWidgetItem * item = NULL;
    
    tree->clear();
    tree = m_Whitelist;
    
    for(int i = 0; i < m_WhitelistState.size() && i < tree->topLevelItemCount(); i++)
    {
        item = tree->topLevelItem(i);
        item->setCheckState(0, getState(m_WhitelistState[i]));
        item->setIcon(0, QIcon());
    }
    
    //remove added whitelist items
    for(int i = m_WhitelistState.size(); i < tree->topLevelItemCount(); i++)
        tree->takeTopLevelItem(i);
}*/

void GuiOptions::undo()
{
    QTreeWidget * tree;

    m_Window->getStartAtBootBox()->setChecked(startAtBoot);
    m_Window->getStartAtBootBox()->setIcon(QIcon());
    m_Window->getAutoListUpdateBox()->setChecked(updateBlocklistsAutomatically);
    m_Window->getAutoListUpdateBox()->setIcon(QIcon());

    getActiveUpdateRadioButton()->setIcon(QIcon());
    updateRadioBtn->setChecked(true);

    //undo whitelist
    tree = m_Window->getWhitelistTreeWidget();
    tree->clear();
    foreach(QTreeWidgetItem *item, m_Whitelist)
    {
        QTreeWidgetItem * item2 = new QTreeWidgetItem(*item);
        tree->addTopLevelItem(item2);
    }
      
    //undo blocklists
    tree = m_Window->getBlocklistTreeWidget();
    tree->clear();
    foreach(QTreeWidgetItem *item, m_Blocklist)
    {
        QTreeWidgetItem * item2 = new QTreeWidgetItem(*item);
        tree->addTopLevelItem(item2);
    }
}

