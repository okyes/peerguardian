
#include "pgl_whitelist.h"
#include "utils.h"
#include "pgl_settings.h"
#include "file_transactions.h"
#include <QDebug>




WhitelistItem::WhitelistItem(const QString& value, const QString& connType, const QString& prot, int type)
{
    m_Value = value;
    m_values << value;
    m_Protocol = prot;
    m_Connection = connType;
    m_Valid = true;

    if ( type == ENABLED )
        m_Enabled = true;
    else if ( type == DISABLED )
        m_Enabled = false;
    else
        m_Valid = false;

}

bool WhitelistItem::operator==(const WhitelistItem& otherItem)
{

    if ( m_Protocol != otherItem.protocol() )
        return false;

    if ( m_Connection != otherItem.connection() )
        return false;

    bool equal = false;

    //the aliases also contain the value
    foreach(QString value, m_values)
        foreach(QString other_value, otherItem.values())
            if ( value == other_value )
                equal = true;

    return equal;
}

void WhitelistItem::addAlias(const QString & alias )
{
    if ( alias.isEmpty() )
        return;

    m_values << alias;
}

void WhitelistItem::addAliases(const QStringList & aliases)
{
    foreach(QString alias, aliases)
        if ( ! m_values.contains(alias, Qt::CaseInsensitive) )
            m_values << alias;

}


Port::Port(QString desig, QString prot, int n)
{
    m_values << desig;

    //the port number can be consider an alias too
    if ( n > 0 )
        m_values << QString::number(n);

    m_protocols << prot;
    m_number = n;
}

Port::Port()
{
    m_number = 0;
}

QString Port::value()
{
    if ( ! m_values.isEmpty() )
        return m_values[0];
    else
        return QString("");
}


Port::Port(const Port& other)
{
    *this = other;
}

void Port::addProtocols(const QStringList& protocols)
{
    m_protocols << protocols;
    m_protocols.removeDuplicates();
}

void Port::addAlias(const QString& alias)
{
    if ( ! m_values.contains(alias) )
        m_values << alias;
}

Port& Port::operator=(const Port& other)
{
    m_number = other.number();
    m_protocols = other.protocols();
    m_values = other.values();
}

bool Port::operator==( WhitelistItem& item)
{

    foreach(QString value, m_values)
        if ( value == item.value() )
            return true;

    return false;
}


PglWhitelist::PglWhitelist(QSettings* settings)
{
    m_WhitelistFile = PglSettings::getStoredValue("CMD_CONF");
    m_Settings = settings;

    if ( m_WhitelistFile.isEmpty() )
        return;

    m_Group[WHITE_IP_IN] = TYPE_INCOMING;
    m_Group[WHITE_IP_OUT] = TYPE_OUTGOING;
    m_Group[WHITE_IP_FWD] = TYPE_FORWARD;
    m_Group[WHITE_TCP_IN] = TYPE_INCOMING;
    m_Group[WHITE_UDP_IN] = TYPE_INCOMING;
    m_Group[WHITE_TCP_OUT] = TYPE_OUTGOING;
    m_Group[WHITE_UDP_OUT] = TYPE_OUTGOING;
    m_Group[WHITE_TCP_FWD] = TYPE_FORWARD;
    m_Group[WHITE_UDP_FWD] = TYPE_FORWARD;

    load();

}

void PglWhitelist::load()
{
    QStringList fileData = getFileData(m_WhitelistFile);

    foreach(QString line, fileData)
    {
        if( line.startsWith("#") )
            continue;

        foreach(QString key, m_Group.keys())
        {
            if ( line.contains(key) )
            {
                m_WhitelistEnabled[key] = getValue(line).split(" ", QString::SkipEmptyParts);
                break;
            }
        }
    }

    //Since disabled whitelisted items (IPs or Ports) can't be easily stored
    //in /etc/plg/pglcmd.conf, the best option is to store them on the GUI settings file
    QString disabled_items;

    foreach ( QString key, m_Group.keys() )
    {
        disabled_items = m_Settings->value(QString("whitelist/%1").arg(key)).toString();
        m_WhitelistDisabled[key] = disabled_items.split(" ", QString::SkipEmptyParts);
    }
}

QString PglWhitelist::getProtocol(QString& key)
{
    return key.split("_")[1];
}

QString PglWhitelist::getTypeAsString(QString& key)
{
    switch(m_Group[key])
    {
        case TYPE_INCOMING: return QString("Incoming");
        case TYPE_OUTGOING: return QString("Outgoing");
        case TYPE_FORWARD: return QString("Forward");
        default: return QString("");
    }

}

QString PglWhitelist::getGroup(QStringList& info)
{
    /*info should contain the value (a port or an ip address) and the connection type (in, out or fwd)*/
    if ( info.size() != 3 )
        return "";

    QMap<QString, QString> connection;
    connection["Incoming"] = "IN";
    connection["Outgoing"] = "OUT";
    connection["Forward"] = "FWD";

    QString value = info[0];
    QString conn_type = info[1];
    QString protocol = info[2];
    QString key = "WHITE_";

    if ( isValidIp(value) )
        key += "IP_";
    else
        key += protocol + "_";


    key += connection[conn_type];

    return key;

}

QStringList PglWhitelist::updateWhitelistFile()
{

    QStringList fileData = getFileData(m_WhitelistFile);
    QString value;

    foreach(QString key, m_WhitelistEnabled.keys())
    {
        value = m_WhitelistEnabled[key].join(" ");

        if ( hasValueInData(key, fileData) || value != PglSettings::getStoredValue(key) )
            replaceValueInData(fileData, key, m_WhitelistEnabled[key].join(" "));
    }

    return fileData;
    //QString filepath = "/tmp/" + m_WhitelistFile.split("/").last();
    //saveFileData(newData, filepath);
}


void PglWhitelist::addTreeWidgetItemToWhitelist(QTreeWidgetItem* treeItem)
{
    QStringList info;
    QString group;
    info << treeItem->text(0) << treeItem->text(1) << treeItem->text(2);

    group = getGroup(info);

    if ( m_Group.contains(group) )
        m_WhitelistDisabled[group] << treeItem->text(0);
}



void PglWhitelist::updateSettings(const QList<QTreeWidgetItem*>& treeItems, int firstAddedItemPos, bool updateAll)
{
    int whitelistDisabledSize = m_WhitelistDisabled.keys().size();
    QTreeWidgetItem * treeItem;
    
    foreach ( QString key, m_WhitelistDisabled.keys() )
        m_WhitelistDisabled[key] = QStringList();
    
    if ( updateAll )
    {
        foreach(QTreeWidgetItem* treeItem, treeItems)
        {
            //if ( treeItem->checkState(0) == Qt::Unchecked && treeItem->icon(0).isNull() ||
             //   treeItem->checkState(0) == Qt::Checked && ( ! treeItem->icon(0).isNull()))
             if ( treeItem->checkState(0) == Qt::Unchecked )
                addTreeWidgetItemToWhitelist(treeItem);
        }
    }
    else if (firstAddedItemPos > 0 ) //added items
    {
        //add older items
        //We don't go through the newly added items here, because their state can be
        //misleading.
        for(int i=0; i < firstAddedItemPos; i++)
        {
            treeItem = treeItems[i];
            if ( treeItem->checkState(0) == Qt::Unchecked && treeItem->icon(0).isNull() ||
                treeItem->checkState(0) == Qt::Checked && ( ! treeItem->icon(0).isNull()))
                    addTreeWidgetItemToWhitelist(treeItem);
        }        

        //Special case for newly added items.
        for(int i=firstAddedItemPos; i < treeItems.size(); i++)
        {
            if ( treeItems[i]->checkState(0) == Qt::Unchecked )
            {
                addTreeWidgetItemToWhitelist(treeItems[i]);
                treeItems[i]->setIcon(0, QIcon());
            }
        }
    }
    
    //write changes to settings' file
    foreach(QString key, m_WhitelistDisabled.keys() )
        if ( m_WhitelistDisabled[key].isEmpty() )
             m_Settings->remove(QString("whitelist/%1").arg(key));
        else
            m_Settings->setValue(QString("whitelist/%1").arg(key), m_WhitelistDisabled[key].join(" "));
}


QStringList PglWhitelist::updatePglcmdConf(QList<QTreeWidgetItem*> treeItems)
{
    if ( m_WhitelistFile.isEmpty() )
        return QStringList();

    QStringList fileData;
    QStringList info;
    QString group;

    /*********** Update the Enabled Whitelist ***************/
    foreach ( QString key, m_WhitelistEnabled.keys() )
        m_WhitelistEnabled[key] = QStringList();

    foreach(QTreeWidgetItem* treeItem, treeItems)
    {
        if ( treeItem->checkState(0) == Qt::Unchecked )
            continue;

        info << treeItem->text(0) << treeItem->text(1) << treeItem->text(2);

        group = getGroup(info);

        if ( m_Group.contains(group) )
            m_WhitelistEnabled[group] << treeItem->text(0);

        info.clear();
    }

    fileData = updateWhitelistFile();


    return fileData;
}

QString PglWhitelist::translateConnection(const QString& conn_type)
{

    QString conn(conn_type.toUpper());
    
    if ( conn == "INCOMING")
        return "IN";
    else if ( conn == "OUTGOING" )
        return "OUT";
    else if ( conn == "FORWARD" )
        return "FWD";
    
    return conn;

}

QStringList PglWhitelist::getDirections(const QString& chain)
{
    QStringList directions;
    
    if ( chain == "IN" )
        directions << QString("--source");
    else if ( chain == "OUT")
        directions << QString("--destination");
    else if ( chain == "FWD" )
        directions << QString("--source") << QString("--destination");
        
    return directions;

}

QStringList PglWhitelist::getCommands( QStringList items, QStringList connections, QStringList protocols, QList<bool> allows)
{

    QStringList commands;
    QString option;
    QString command_type("");
    QString command;
    QString chain, item, connection, protocol, conn;
    QStringList directions;
    bool ip;
    QString iptables_target_whitelisting = PglSettings::getStoredValue("IPTABLES_TARGET_WHITELISTING");
    
    
    for (int i=0; i < items.size(); i++ )
    {
        item = items[i];
        connection = connections[i];
        protocol = protocols[i];
        
        
        if ( isValidIp(item) )
        {
            command_type = "iptables $OPTION $IPTABLES_CHAIN $FROM $IP -j " + iptables_target_whitelisting;
            ip = true;
        }
        else
        {
            command_type = "iptables $OPTION $IPTABLES_CHAIN -p $PROT --dport $PORT -j " + iptables_target_whitelisting;
            ip = false;
        }
        
        if ( allows[i] )
            option = "-I";
        else
            option = "-D";
        
        //convert incoming to in, outgoing to out, etc
        conn = translateConnection(connection);
        chain = PglSettings::getStoredValue("IPTABLES_" + conn);
        
        //FWD needs --source and --destination
        if ( ip )
            directions = getDirections(conn);
        else
        {
            protocol = protocol.toLower();
            if ( protocol != "tcp" && protocol != "udp")
                continue;
        }
        
        if ( ip )
        {      
            foreach( QString direction, directions )
            {
                command = QString(command_type);
                command.replace(QString("$OPTION"), option);
                command.replace(QString("$IPTABLES_CHAIN"), chain);
                command.replace(QString("$FROM"), direction);
                command.replace(QString("$IP "), item + " ");
                commands << command;
            }
        }
        else
        {
            command = QString(command_type);
            command.replace(QString("$OPTION"), option);
            command.replace(QString("$IPTABLES_CHAIN"), chain);
            command.replace(QString("$PROT"), protocol);
            command.replace(QString("$PORT"), item);
            commands << command;
        }
        
        
    }

    
    return commands;
        
}

