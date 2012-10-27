
#include "pgl_whitelist.h"
#include "utils.h"
#include "pgl_settings.h"
#include "file_transactions.h"
#include "gui_options.h"

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

PglWhitelist::PglWhitelist(QSettings* settings, GuiOptions * guiOptions)
{
    m_WhitelistFile = PglSettings::getStoredValue("CMD_CONF");
    m_Settings = settings;
    m_GuiOptions = guiOptions;

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
    
    if ( ! m_WhitelistEnabled.isEmpty() )
        m_WhitelistEnabled.clear();

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
    
    if ( ! m_WhitelistDisabled.isEmpty() )
        m_WhitelistDisabled.clear();

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
    else if (firstAddedItemPos >= 0 ) //added items
    {
        //add older items
        //We don't go through the newly added items here, because their state can be
        //misleading.
        for(int i=0; i < firstAddedItemPos; i++)
        {
            treeItem = treeItems[i];
            if ( (treeItem->checkState(0) == Qt::Unchecked && treeItem->icon(0).isNull() ) ||
                (treeItem->checkState(0) == Qt::Checked && ( ! treeItem->icon(0).isNull())))
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


QStringList PglWhitelist::updateWhitelistItemsInIptables(QList<QTreeWidgetItem*> items, GuiOptions *guiOptions)
{
    QStringList values, connections, protocols;
    QList<bool> allows;
    QList<QTreeWidgetItem*> removedItems = guiOptions->getRemovedWhitelistItemsForIptablesRemoval();
    int firstAddedItem = guiOptions->getPositionFirstAddedWhitelistItem();
    int totalPrevItems = guiOptions->getWhitelistPrevSize();
    QTreeWidgetItem * item;
    
    //append removed items
    if ( ! removedItems.isEmpty() )
    {
        foreach(item, removedItems)
        {
            values << item->text(0);
            connections << item->text(1);
            protocols << item->text(2);
            allows << false;
        }
    }
    
    //append added items
    if ( (totalPrevItems > 0 && firstAddedItem > 0)  || (totalPrevItems == 0 && firstAddedItem == 0) )
    {
        for(int i=firstAddedItem; i < items.size(); i++)
        {
            item = items[i];
            if ( item->checkState(0) == Qt::Checked  )
            {
                values << item->text(0);
                connections << item->text(1);
                protocols << item->text(2);
                allows << true;
            }
        }
        
    }
    else 
        firstAddedItem = items.size();
    
    for(int i=0; i < firstAddedItem; i++)
    {
        item = items[i];
    
        //if it has a warning icon and is not one of the newly added items
        if ( ! item->icon(0).isNull() )
        {
            values << item->text(0);
            connections << item->text(1);
            protocols << item->text(2);

            if ( item->checkState(0) == Qt::Checked )
                allows << true;
            else
                allows << false;
        }
    }
    
    return getCommands(values, connections, protocols, allows);
        
}

QStringList PglWhitelist::getCommands( QStringList items, QStringList connections, QStringList protocols, QList<bool> allows)
{

    QStringList commands;
    QString option;
    QString command_operator;
    QString ip_source_check, ip_destination_check, port_check;
    QString command_type("");
    QString command, iptables_list_type("iptables -L $IPTABLES_CHAIN -n | ");
    QString chain, item, connection, protocol, conn, iptables_list, checkCmd;
    QStringList directions;
    QString portNumber("0");
    bool ip;
    QString iptables_target_whitelisting = PglSettings::getStoredValue("IPTABLES_TARGET_WHITELISTING");
    QString ip_source_check_type = QString("grep -x \'%1 *all *-- *$IP *0.0.0.0/0 *\'").arg(iptables_target_whitelisting);
    QString ip_destination_check_type = QString("grep -x \'%1 *all *-- *0.0.0.0/0 *$IP *\'").arg(iptables_target_whitelisting);
    QString port_check_type = QString("grep -x \'%1 *$PROTO *-- *0.0.0.0/0 *0.0.0.0/0 *$PROTO dpt:$PORT *\'").arg(iptables_target_whitelisting);

    
    for (int i=0; i < items.size(); i++ )
    {
        item = items[i];
        connection = connections[i];
        protocol = protocols[i];
        iptables_list = iptables_list_type;
        int _port = 0;
        
        ip = isValidIp(item);
        
        if (! ip ) {
          _port = port(item);
          if (_port != -1)
            portNumber = QString::number(_port);
          else
            portNumber = item;
        }
        
        if ( ip )
        {
            command_type = " $COMMAND_OPERATOR iptables $OPTION $IPTABLES_CHAIN $FROM $IP -j " + iptables_target_whitelisting;
            // NOTE jre: IN uses source, OUT uses destination, and FWD uses both.
            // So for IN and OUT we have excess functions set here:
            ip_source_check = ip_source_check_type;
            ip_destination_check = ip_destination_check_type;
            ip_source_check.replace("$IP", item);
            ip_destination_check.replace("$IP", item);
        }
        else
        {
            command_type = " $COMMAND_OPERATOR iptables $OPTION $IPTABLES_CHAIN -p $PROT --dport $PORT -j " + iptables_target_whitelisting;
            port_check = port_check_type;
            port_check.replace("$PROTO", protocol.toLower());
            port_check.replace("$PORT", portNumber);
        }
        
        if ( allows[i] )
        {
            option = "-I";
            command_operator = "||";
        }
        else
        {
            option = "-D";
            command_operator = "&&";
        }
        
        //convert incoming to in, outgoing to out, etc
        conn = translateConnection(connection);
        //get iptables chain name from pglcmd.defaults
        chain = PglSettings::getStoredValue("IPTABLES_" + conn);
        
        iptables_list.replace("$IPTABLES_CHAIN", chain);
        
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
                command.replace(QString("$COMMAND_OPERATOR"), command_operator);
                command.replace(QString("$IPTABLES_CHAIN"), chain);
                command.replace(QString("$FROM"), direction);
                command.replace(QString("$IP "), item + " ");

                if ( direction == "--source" )
                    commands << iptables_list + ip_source_check + command;
                else if ( direction == "--destination")
                    commands << iptables_list + ip_destination_check + command;

            }
        }
        else
        {
            command = QString(command_type);
            command.replace(QString("$OPTION"), option);
            command.replace(QString("$COMMAND_OPERATOR"), command_operator);
            command.replace(QString("$IPTABLES_CHAIN"), chain);
            command.replace(QString("$PROT"), protocol);
            command.replace(QString("$PORT"), portNumber);
            commands << iptables_list + port_check + command;
        }
    }
    
    return commands;
        
}

bool PglWhitelist::isPortAdded(const QString& value, const QString & portRange)
{
    bool ok = false;
    
    if ( portRange.contains(":") )
    {
        int part1 = portRange.split(":")[0].toInt(&ok);
        if ( ! ok ) return false;
        int part2 = portRange.split(":")[1].toInt(&ok);
        if ( ! ok ) return false;
        int val = value.toInt(&ok);
        if ( ! ok ) return false;
        
        if ( val >= part1 && val <= part2 )
            return true;
    }
    
    return false;
}

QString PglWhitelist::getIptablesTestCommand(const QString& value, const QString& connectType, const QString& prot)
{
    QString cmd("");
    QString target = PglSettings::getStoredValue("IPTABLES_TARGET_WHITELISTING");
    QString chain;
    QString portNumber = value;
    int _port = 0;
    
    chain = translateConnection(connectType);
    chain = PglSettings::getStoredValue("IPTABLES_" + chain);
    
    if ( isValidIp(value) )
    {
        cmd = QString("iptables -L \"$CHAIN\" -n | grep \"$TARGET\" | grep -F \"$VALUE\"");
    }
    else
    {
        cmd = QString("iptables -L \"$CHAIN\" -n | grep \"$TARGET\" | grep \"$PROT dpt:$VALUE\"");
        cmd.replace("$PROT", prot);
        _port = port(value);
        if (_port != -1)
          portNumber = QString::number(_port);
    }
    
    cmd.replace("$CHAIN", chain);
    cmd.replace("$TARGET", target);
    cmd.replace("$VALUE", portNumber);

    return cmd;
}

bool PglWhitelist::isInPglcmd(const QString& value, const QString& connectType, const QString& prot)
{
    QList<QTreeWidgetItem*> items = m_GuiOptions->getWhitelist();
    QString protocol = prot;

    if ( isValidIp(value) )
        protocol = "IP";

    foreach(QTreeWidgetItem*item, items)
    {    
        if ( connectType == item->text(1) && protocol == item->text(2) )
        {
            if ( value == item->text(0) )
            {
                return true;
            }
            else if ( isPortAdded(value, item->text(0)) ) //could be a port range
            {
                return true;
            }
        }
    }
    
    return false;
}

