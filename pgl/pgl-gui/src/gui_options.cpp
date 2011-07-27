#include "gui_options.h"

bool GuiOptions::isChanged()
{

    if (startAtBoot != m_Window->getStartAtBootBox()->isChecked() )
        return true;

    if (updateBlocklistsAutomatically != m_Window->getAutoListUpdateBox()->isChecked() )
        return true;

    if ( updateRadioBtnName != getActiveUpdateRadioButtonName() )
        return true;

    if ( listStateChanged(m_Window->getBlocklistTreeWidget()) )
        return true;

    if ( listStateChanged(m_Window->getWhitelistTreeWidget()) )
        return true;

    return false;
}

bool GuiOptions::hasToUpdatePglcmdConf()
{
    if (startAtBoot != m_Window->getStartAtBootBox()->isChecked() )
        return true;

    if (updateBlocklistsAutomatically != m_Window->getAutoListUpdateBox()->isChecked() )
        return true;

    if ( listStateChanged(m_Window->getWhitelistTreeWidget()) )
        return true;

    return false;
}

bool GuiOptions::hasToUpdateBlocklistList()
{
    if ( listStateChanged(m_Window->getBlocklistTreeWidget()) )
        return true;

    return false;
}

bool GuiOptions::listStateChanged(QTreeWidget * tree)
{
    QList<int> listState;

    if ( tree->objectName().contains("Blocklist", Qt::CaseInsensitive) )
        listState = m_BlocklistState;
    else
        listState = m_WhitelistState;

    //test if new items were added
    if ( tree->topLevelItemCount() != listState.size() )
        return true;

    for ( int i=0; i < tree->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem * item = tree->topLevelItem(i);

        if ( item->checkState(0) != listState[i] )
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

void GuiOptions::updateList(QTreeWidget * tree)
{
    QList<int> listState;

    for ( int i=0; i < tree->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem * item = tree->topLevelItem(i);
        listState.append(item->checkState(0));
    }

    if ( tree->objectName().contains("Blocklist", Qt::CaseInsensitive) )
        m_BlocklistState = listState;
    else
        m_WhitelistState = listState;

}

void GuiOptions::update()
{
    updateStartAtBoot();
    updateUpdateRadioBtn();
    updateList(m_Window->getBlocklistTreeWidget());
    updateList(m_Window->getWhitelistTreeWidget());
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

void GuiOptions::undoBlocklist()
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
}

void GuiOptions::undoWhitelist()
{
    //reset the blocklist and whitelist treewidgets
    QTreeWidget * tree = m_Window->getWhitelistTreeWidget();
    QTreeWidgetItem * item = NULL;
    
    for(int i = 0; i < m_WhitelistState.size(); i++)
    {
        item = tree->topLevelItem(i);
        item->setCheckState(0, getState(m_WhitelistState[i]));
        item->setIcon(0, QIcon());
    }
    
    //remove added whitelist items
    for(int i = m_WhitelistState.size(); i < tree->topLevelItemCount(); i++)
        tree->takeTopLevelItem(i);
}

void GuiOptions::undo()
{
    m_Window->getStartAtBootBox()->setChecked(startAtBoot);
    m_Window->getStartAtBootBox()->setIcon(QIcon());
    m_Window->getAutoListUpdateBox()->setChecked(updateBlocklistsAutomatically);
    m_Window->getAutoListUpdateBox()->setIcon(QIcon());

    getActiveUpdateRadioButton()->setIcon(QIcon());
    updateRadioBtn->setChecked(true);

    undoBlocklist();
    undoWhitelist();
    
}

