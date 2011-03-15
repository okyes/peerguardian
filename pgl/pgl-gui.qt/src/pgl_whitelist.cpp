
#include "pgl_whitelist.h"
#include "utils.h"
#include "pgl_settings.h"
#include "file_transactions.h"
#include <QDebug>


WhitelistItem::WhitelistItem(QString& value, int connection, bool enabled, QString& group)
{
    m_Value = value;
    
    m_Type = PORT;
    /*if ( isValidIp(value) )
        m_Type = IP;
    else if ( getPort(value)  != -1 )
        m_Type = PORT;
    else
        m_Type = INVALID;*/
    
    m_Transport = TCP;
    m_Connection = connection;
    m_Group = group;
    m_Enabled = enabled;
}

QString WhitelistItem::getConnectionAsString()
{
    switch(m_Connection)
    {
        case TYPE_INCOMING: return QString("Incoming");
        case TYPE_OUTGOING: return QString("Outgoing");
        case TYPE_FORWARD: return QString("Forward"); 
        default: return QString("");
    }
}

QString WhitelistItem::getTypeAsString()
{
    switch(m_Connection)
    {
        case IP: return QString("IP");
        case PORT: return QString("Port");
        default: return QString("");
    }
}

PglWhitelist::PglWhitelist(QSettings* settings)
{
    m_WhitelistFile = getVariable(PGLCMD_DEFAULTS_PATH, "CONFDIR") + QString("/");
    m_WhitelistFile += getVariable(PGLCMD_DEFAULTS_PATH, "CMD_NAME") + QString(".conf");
    
    m_Settings = settings;
    
    if ( m_WhitelistFile.isEmpty() )
        return;
        
    m_WhitelistConnection[WHITE_IP_IN] = TYPE_INCOMING;
    m_WhitelistConnection[WHITE_IP_OUT] = TYPE_OUTGOING;
    m_WhitelistConnection[WHITE_IP_FWD] = TYPE_FORWARD;
    m_WhitelistConnection[WHITE_TCP_IN] = TYPE_INCOMING;
    m_WhitelistConnection[WHITE_UDP_IN] = TYPE_INCOMING;
    m_WhitelistConnection[WHITE_TCP_OUT] = TYPE_OUTGOING;
    m_WhitelistConnection[WHITE_UDP_OUT] = TYPE_OUTGOING;
    m_WhitelistConnection[WHITE_TCP_FWD] = TYPE_FORWARD;
    m_WhitelistConnection[WHITE_UDP_FWD] = TYPE_FORWARD;
    
    QStringList fileData = getFileData(m_WhitelistFile);
    
    foreach(QString line, fileData)
    {
        if( line.startsWith("#") )
            continue;
        
        foreach(QString key, m_WhitelistConnection.keys())
        {
            if ( line.contains(key) )
            {
                QString variable = getVariable(line);
                m_WhitelistEnabled[key] = variable.split(" ", QString::SkipEmptyParts);
                
                break;
            }
        }
    }
    
    //Since disabled whitelisted items (IPs or Ports) can't be easily stored
    //in /etc/plg/pglcmd.conf, the best option is to store them on the GUI settings file
    QString disabled_items;
    
    foreach ( QString key, m_WhitelistConnection.keys() )
    {
        disabled_items = settings->value(QString("whitelist/%1").arg(key)).toString();
        m_WhitelistDisabled[key] = disabled_items.split(" ", QString::SkipEmptyParts);
    }
}

QList<WhitelistItem> PglWhitelist::getWhitelistItems(QList<QTreeWidgetItem*> treeItems)
{
    QList<WhitelistItem> disabledItems;
    
    foreach(QTreeWidgetItem* treeItem, treeItems)
        foreach(WhitelistItem whiteItem, m_WhitelistedItems)
            if ( whiteItem.value() == treeItem->text(0) )
                disabledItems << whiteItem;

    return disabledItems;
}

void PglWhitelist::disableItems(QList<QTreeWidgetItem*> treeItems)
{
    if ( m_WhitelistFile.isEmpty() )    
        return;
    
    QList<WhitelistItem> disabledItems = getWhitelistItems(treeItems);
    
    
    QStringList valuesToDelete;
    foreach(QTreeWidgetItem* treeItem, treeItems)
        valuesToDelete << treeItem->text(0);
    
    //Remove data from pglcmd.conf
    QStringList fileData = getFileData(m_WhitelistFile);
    QStringList newData;
    QStringList values;
    
    foreach(QString key, m_WhitelistConnection.keys())
    {
        values = m_WhitelistEnabled[key];
        
        foreach ( QString valueToDelete, valuesToDelete )
            if ( values.contains(valueToDelete) )
                values.removeAll(valueToDelete);
        
        m_WhitelistEnabled[key] = values;
    }
    
    
    foreach(QString line, fileData)
    {
        if ( line.startsWith("#") || line.isEmpty() )
        {
            newData << line;
            continue;
        }
        
        foreach(QString key, m_WhitelistEnabled.keys())
        {
            if ( line.contains( key ) )
                newData << key + "=\"" + m_WhitelistEnabled[key].join(" ") + "\""; 
        }        
        
    }
    
    //FIXME: Needs sudo rights to write back to /etc/pgl/pglcmd.conf
    saveFileData(newData, "/tmp/pglcmd.conf");
    
    //Add data to peerguardian.ini
    /*foreach( WhitelistItem, items )
        if ( ! item.isEnabled() )
            m_Settings->setValue("whitelist/");*/
    
}

