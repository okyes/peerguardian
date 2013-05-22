/***************************************************************************
 *   Copyright (C) 2013 by Carlos Pais <freemind@lavabit.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
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

#include "pgl_whitelist.h"
#include "utils.h"
#include "pgl_settings.h"
#include "file_transactions.h"
#include "gui_options.h"

WhitelistManager::WhitelistManager(QSettings* settings)
{
    m_WhitelistFile = PglSettings::value("CMD_CONF");
    m_Settings = settings;
    m_GuiOptions = 0;

    //if ( m_WhitelistFile.isEmpty() )
    //    return;

    m_Group[WHITE_IP_IN] = TYPE_INCOMING;
    m_Group[WHITE_IP_OUT] = TYPE_OUTGOING;
    m_Group[WHITE_IP_FWD] = TYPE_FORWARD;
    m_Group[WHITE_TCP_IN] = TYPE_INCOMING;
    m_Group[WHITE_UDP_IN] = TYPE_INCOMING;
    m_Group[WHITE_TCP_OUT] = TYPE_OUTGOING;
    m_Group[WHITE_UDP_OUT] = TYPE_OUTGOING;
    m_Group[WHITE_TCP_FWD] = TYPE_FORWARD;
    m_Group[WHITE_UDP_FWD] = TYPE_FORWARD;

    loadSystemPorts();
}

WhitelistManager::~WhitelistManager()
{
    foreach(WhitelistItem* item, mWhiteListItems)
        if (item)
            delete item;
    mWhiteListItems.clear();
}

void WhitelistManager::load()
{
    QStringList fileData = getFileData(m_WhitelistFile);
    QStringList values;
    QString key, skey, value;

    if ( ! m_WhitelistEnabled.isEmpty() )
        m_WhitelistEnabled.clear();

    foreach(WhitelistItem* item, mWhiteListItems)
        if (item)
            delete item;
    if (! mWhiteListItems.isEmpty())
        mWhiteListItems.clear();

    foreach(const QString& line, fileData)
    {
        if( line.startsWith("#") )
            continue;

        key = getVariable(line);
        if (m_Group.contains(key)) {
            values = getValue(line).split(" ", QString::SkipEmptyParts);
            foreach(const QString& value, values)
                mWhiteListItems.append(new WhitelistItem(value, parseConnectionType(key), parseProtocol(key), true));

            m_WhitelistEnabled[key] = values;
        }

        /*foreach(const QString& key, m_Group.keys()) {
            if ( line.contains(key) ) {
                values = getValue(line).split(" ", QString::SkipEmptyParts);

                foreach(const QString& value, values)
                    mWhiteListItems.append(new WhitelistItem(value, parseProtocol(line), parseConnectionType(line)));

                m_WhitelistEnabled[key] = getValue(line).split(" ", QString::SkipEmptyParts);
                break;
            }
        }*/
    }

    //Since disabled whitelisted items (IPs or Ports) can't be easily stored
    //in /etc/plg/pglcmd.conf, the best option is to store them on the GUI settings file
    
    if ( ! m_WhitelistDisabled.isEmpty() )
        m_WhitelistDisabled.clear();

    foreach (key, m_Group.keys() ) {
        skey = QString("whitelist/%1").arg(key);
        if (m_Settings->contains(skey)) {
            value = m_Settings->value(skey).toString();
            values = value.split(" ", QString::SkipEmptyParts);
            foreach(const QString& value, values) {
                WhitelistItem* item = new WhitelistItem(value, parseConnectionType(key), parseProtocol(key), true, false);
                qDebug() << item->isRemoved();
                mWhiteListItems.append(item);
            }
            m_WhitelistDisabled[key] = values;
        }
    }
}

QString WhitelistManager::parseProtocol(const QString& key)
{
    QRegExp keyPattern("^[a-zA-Z]+_[a-zA-Z]+_[a-zA-Z]+$");
    if (keyPattern.exactMatch(key))
        return key.split("_")[1];
    return key;
}

QString WhitelistManager::parseConnectionType(const QString& key)
{
    QRegExp keyPattern("^[a-zA-Z]+_[a-zA-Z]+_[a-zA-Z]+$");
    if (keyPattern.exactMatch(key))
        return getTypeAsString(key);
    return key;
}


QString WhitelistManager::getTypeAsString(const QString& key)
{
    switch(m_Group[key])
    {
        case TYPE_INCOMING: return QString("Incoming");
        case TYPE_OUTGOING: return QString("Outgoing");
        case TYPE_FORWARD: return QString("Forward");
        default: return QString("");
    }
}

QString WhitelistManager::getGroup(QStringList& info)
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

QStringList WhitelistManager::updateWhitelistFile()
{

    QStringList fileData = getFileData(m_WhitelistFile);
    QString value;

    foreach(QString key, m_WhitelistEnabled.keys())
    {
        value = m_WhitelistEnabled[key].join(" ");

        if ( hasValueInData(key, fileData) || value != PglSettings::value(key) )
            replaceValueInData(fileData, key, m_WhitelistEnabled[key].join(" "));
    }

    return fileData;
    //QString filepath = "/tmp/" + m_WhitelistFile.split("/").last();
    //saveFileData(newData, filepath);
}


/*void WhitelistManager::addTreeWidgetItemToWhitelist(QTreeWidgetItem* treeItem)
{
    QStringList info;
    QString group;
    info << treeItem->text(0) << treeItem->text(1) << treeItem->text(2);

    group = getGroup(info);

    if ( m_Group.contains(group) )
        m_WhitelistDisabled[group] << treeItem->text(0);
}

void WhitelistManager::updateSettings(const QList<QTreeWidgetItem*>& treeItems, int firstAddedItemPos, bool updateAll)
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
}*/

bool WhitelistManager::isChanged()
{
    foreach(WhitelistItem* item, mWhiteListItems)
        if (item && item->isChanged())
            return true;
    return false;
}

void WhitelistManager::updatePglSettings()
{
    QStringList info;
    QString group;

    foreach(WhitelistItem* item, mWhiteListItems) {
        if (item->isChanged()) {
            info << item->value() << item->connection() << item->protocol();
            group = getGroup(info);
            if (item->isEnabled()) {
                PglSettings::add(group, item->value());
            }
            else if (item->isDisabled() || item->isRemoved()) {
                PglSettings::remove(group, item->value());
            }

            info.clear();
        }
    }

    qDebug() << "WHITE_TCP_OUT" << PglSettings::value("WHITE_TCP_OUT");
    /*foreach(const QString& key, groups.keys()) {
        PglSettings::store(key, groups[key]);
    }*/
}

void WhitelistManager::updateGuiSettings()
{
    QString key, group;
    QStringList info, values;

    foreach(WhitelistItem* item, mWhiteListItems) {
        if (item->isChanged()) {
            info << item->value() << item->connection() << item->protocol();
            group = getGroup(info);
            key = QString("whitelist/%1").arg(group);
            if (m_Settings->contains(key))
                values = m_Settings->value(key).toString().split(" ", QString::SkipEmptyParts);

            qDebug() << item->value() << item->isRemoved() << item->isDisabled() << group << values;

            if (item->isEnabled() || item->isRemoved()) {
                if (values.contains(item->value()))
                    values.removeAll(item->value());
            }
            else if (item->isDisabled() && ! values.contains(item->value())) {
                values += item->value();
            }

            qDebug() << "VALUES:" << values;
            if (values.isEmpty())
                m_Settings->remove(key);
            else
                m_Settings->setValue(key, values.join(" "));
            values.clear();
        }
        /*if (item->isDisabled()) {
            QStringList info; info << item->value() << item->connection() << item->protocol();
            groups[getGroup(info)] += item->value();
            info.clear();
        }*/
    }

    //write changes to settings' file
    /*foreach(const QString& group, m_Group.keys() ) {
        key = QString("whitelist/%1").arg(group);
        if (! groups.contains(group) && m_Settings->contains(group))
            m_Settings->remove(group);
        else if (groups.contains(group))
            m_Settings->setValue(group, groups[group]);
    }*/
}



/*QStringList WhitelistManager::updatePglcmdConf(QList<QTreeWidgetItem*> treeItems)
{
    if ( m_WhitelistFile.isEmpty() )
        return QStringList();

    QStringList fileData;
    QStringList info;
    QString group;

    /////////// Update the Enabled Whitelist ////////////
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
}*/

QString WhitelistManager::translateConnection(const QString& conn_type)
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

QStringList WhitelistManager::getDirections(const QString& chain)
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

QStringList WhitelistManager::generateIptablesCommands()
{
    QStringList values, connections, protocols;
    QList<bool> allows;

    foreach(WhitelistItem* item, mWhiteListItems) {

        if (item->isChanged()) {
            values << item->value();
            connections << item->connection();
            protocols << item->protocol();
            if (item->isRemoved())
                allows << false;
            else
                allows << item->isEnabled();
        }
    }

    return getCommands(values, connections, protocols, allows);
}

/*QStringList WhitelistManager::updateWhitelistItemsInIptables(QList<QTreeWidgetItem*> items, GuiOptions *guiOptions)
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
        
}*/

QStringList WhitelistManager::getCommands( QStringList items, QStringList connections, QStringList protocols, QList<bool> allows)
{

    QStringList commands;
    QString option;
    QString command_operator;
    QString ip_source_check, ip_destination_check, port_check;
    QString command_type("");
    QString command, iptables_list_type("iptables -L $IPTABLES_CHAIN -n | ");
    QString chain, item, connection, protocol, conn, iptables_list, checkCmd;
    QStringList directions;
    QString portNum("0");
    bool ip;
    QString iptables_target_whitelisting = PglSettings::value("IPTABLES_TARGET_WHITELISTING");
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
          _port = portNumber(item);
          if (_port != -1)
            portNum = QString::number(_port);
          else
            portNum = item;
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
            port_check.replace("$PORT", portNum);
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
        chain = PglSettings::value("IPTABLES_" + conn);
        
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
            command.replace(QString("$PORT"), portNum);
            commands << iptables_list + port_check + command;
        }
    }
    
    return commands;
        
}

bool WhitelistManager::isPortAdded(const QString& value, const QString & portRange)
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

QString WhitelistManager::getIptablesTestCommand(const QString& value, const QString& connectType, const QString& prot)
{
    QString cmd("");
    QString target = PglSettings::value("IPTABLES_TARGET_WHITELISTING");
    QString chain;
    QString portNum = value;
    int _port = 0;
    
    chain = translateConnection(connectType);
    chain = PglSettings::value("IPTABLES_" + chain);
    
    if ( isValidIp(value) )
    {
        cmd = QString("iptables -L \"$CHAIN\" -n | grep \"$TARGET\" | grep -F \"$VALUE\"");
    }
    else
    {
        cmd = QString("iptables -L \"$CHAIN\" -n | grep \"$TARGET\" | grep \"$PROT dpt:$VALUE\"");
        cmd.replace("$PROT", prot);
        _port = portNumber(value);
        if (_port != -1)
          portNum = QString::number(_port);
    }
    
    cmd.replace("$CHAIN", chain);
    cmd.replace("$TARGET", target);
    cmd.replace("$VALUE", portNum);

    return cmd;
}

bool WhitelistManager::contains(const WhitelistItem & item)
{
    foreach(WhitelistItem* _item, mWhiteListItems)
        if (*_item == item)
            return true;

    return false;
}

bool WhitelistManager::contains(const QString& value, const QString& connectType, const QString& prot)
{
    return contains(WhitelistItem(value, connectType, prot));
}

bool WhitelistManager::isValid(const WhitelistItem & item, QString & reason)
{
    if (contains(item)) {
        reason = QObject::tr("It's already added");
        return false;
    }

    foreach(const Port& port, mSystemPorts) {
        if ( port.containsName(item.value()) ) {
            if ( ! port.hasProtocol(item.protocol()) ) {
                reason += item.value();
                reason += QObject::tr(" doesn't work over ") + item.protocol();
                return false;
            }
        }
    }

    return true;
}

bool WhitelistManager::isValid(const QString& value, const QString& connectType, const QString& prot, QString& reason)
{
    return isValid(WhitelistItem(value, connectType, prot), reason);
}

bool WhitelistManager::isInPglcmd(const QString& value, const QString& connectType, const QString& prot)
{
    QList<QTreeWidgetItem*> items = m_GuiOptions->getWhitelist();
    QString protocol = prot;

    if ( isValidIp(value) )
        protocol = "IP";

    foreach(QTreeWidgetItem*item, items) {
        if ( connectType == item->text(1) && protocol == item->text(2) ) {
            if ( value == item->text(0) )
                return true;
            else if ( isPortAdded(value, item->text(0)) ) //could be a port range
                return true;
        }
    }
    
    return false;
}

QList<WhitelistItem*> WhitelistManager::whitelistItems()
{
    return mWhiteListItems;
}

WhitelistItem* WhitelistManager::whitelistItemAt(int index)
{
    if (index >= 0 && index < mWhiteListItems.size())
        return mWhiteListItems[index];
    return 0;
}

void WhitelistManager::addItem(WhitelistItem * item)
{
    mWhiteListItems.append(item);
}

void WhitelistManager::removeItemAt(int index)
{
    if (index >= 0 && index < mWhiteListItems.size())
        mWhiteListItems[index]->remove();
}

void WhitelistManager::undo()
{
    WhitelistItem* item;

    for(int i=mWhiteListItems.size()-1; i >= 0; i--) {
        item = mWhiteListItems[i];
        if (item->isChanged()) {
            if (item->isAdded()) {
                mWhiteListItems.takeAt(i);
                delete item;
            }
            else
                item->undo();
        }
    }
}

QList<Port> WhitelistManager::systemPorts()
{
    return mSystemPorts;
}

QHash<QString, int> WhitelistManager::systemPortsNameToNumber()
{
  QHash<QString, int> ports;

  foreach(const Port& port, mSystemPorts)
    foreach(const QString& name, port.names())
      if (! isNumber(name))
        ports[name] = port.number();

  return ports;
}

int WhitelistManager::portNumber(const QString& name)
{
  if (isNumber(name))
    return name.toInt();

  foreach(const Port& port, mSystemPorts)
      if (port.containsName(name))
        return port.number();

  return -1;
}

void WhitelistManager::loadSystemPorts()
{
    if (! mSystemPorts.isEmpty())
        mSystemPorts.clear();

    QStringList fileData = getFileData("/etc/services");
    Port port1, port2;

    for ( int i=0; i < fileData.size(); i++ )
    {
        port1 = parsePort(fileData[i]);

        if ( i < fileData.size()-1 )
        {
            //get next line
            port2 = parsePort(fileData[i+1]);

            if ( port1.name() == port2.name() )
            {
                port1.addProtocols(port2.protocols());
                ++i; //ignores next line
            }
        }

        mSystemPorts.append(port1);
    }
}

Port WhitelistManager::parsePort(QString line)
{
    QStringList elements;
    int portNum;
    QString protocol;
    Port port;

    line = line.simplified();

    if ( line.isEmpty() || line.startsWith("#") )
        return Port();

    elements = line.split(" ");

    portNum = elements[1].split("/")[0].toInt();
    protocol = elements[1].split("/")[1];

    port = Port(elements[0], protocol, portNum);

    if ( elements.size() >= 3 && ( ! elements[2].startsWith("#")) )
        port.addName(elements[2]);

    return port;
}

bool WhitelistManager::isPort(const QString & p)
{
    if ( p.contains(":") ) //port range
    {
        QStringList ports = p.split(":");

        if ( ports.size() > 2 )
            return false;

        foreach(QString port, ports)
            if (! isNumber(port))
                return false;

        return true;
    }

    if (isNumber(p))
      return true;

    if (portNumber(p.toLower()) != -1)
      return true;

    return false;
}
