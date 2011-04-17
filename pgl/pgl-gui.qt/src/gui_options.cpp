#include "gui_options.h"

bool GuiOptions::isChanged()
{
    
    if (startAtBoot != m_Window->getStartAtBootBox()->isChecked() )
        return true;
        
    if (updateBlocklistsAutomatically != m_Window->getAutoListUpdateBox()->isChecked() )
        return true;
    
    if ( updateRadioBtn != getActiveUpdateRadioButton() )
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

QString GuiOptions::getActiveUpdateRadioButton()
{
    
    if ( m_Window->getAutoListUpdateDailyRadio()->isChecked() )
        return m_Window->getAutoListUpdateDailyRadio()->objectName();
    else if ( m_Window->getAutoListUpdateWeeklyRadio()->isChecked() )
        return m_Window->getAutoListUpdateWeeklyRadio()->objectName();
    else if ( m_Window->getAutoListUpdateMonthlyRadio()->isChecked() )
        return m_Window->getAutoListUpdateMonthlyRadio()->objectName();
        
    return QString("");
}

/********* Update methods ************/

void GuiOptions::updateUpdateRadioBtn()
{
    updateBlocklistsAutomatically = m_Window->getAutoListUpdateBox()->isChecked();
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
