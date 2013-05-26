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

#ifndef PGL_WHITELIST_H
#define PGL_WHITELIST_H

#include <QMultiMap>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QList>

#include "whitelist_item.h"
#include "utils.h"

#define WHITE_IP_IN "WHITE_IP_IN"
#define WHITE_IP_OUT "WHITE_IP_OUT"
#define WHITE_IP_FWD "WHITE_IP_FWD"
#define WHITE_TCP_IN "WHITE_TCP_IN"
#define WHITE_UDP_IN "WHITE_UDP_IN"
#define WHITE_TCP_OUT "WHITE_TCP_OUT"
#define WHITE_UDP_OUT "WHITE_UDP_OUT"
#define WHITE_TCP_FWD "WHITE_TCP_FWD"
#define WHITE_UDP_FWD "WHITE_UDP_FWD"


class GuiOptions;

class WhitelistManager
{
    QList<WhitelistItem*> mWhitelistItems;
    QList<WhitelistItem*> mRemovedWhitelistItems;
    QString m_WhitelistFile;
    QStringList mWhitelistFileData;
    QMap<QString, int> m_Group;
    QSettings * m_Settings;
    QMap<QString, QStringList> m_WhitelistEnabled;
    QMap<QString, QStringList> m_WhitelistDisabled;
    QList<Port> mSystemPorts;

	public:
		/**
         * Constructor. Creates an emtpy WhitelistManager object with no data loaded.
		 */
        WhitelistManager(QSettings *);
		/**
		 * Destructor.
		 */
        ~WhitelistManager();

        QList<WhitelistItem*> whitelistItems();
        void loadDisabledItems(QSettings*);
        //QStringList updatePglcmdConf(QList<QTreeWidgetItem*>);
        QMap<QString, QStringList> getEnabledWhitelistedItems() { return m_WhitelistEnabled; }
        QMap<QString, QStringList> getDisabledWhitelistedItems(){ return m_WhitelistDisabled; }
        QString getTypeAsString(const QString&);
        QString getGroup(QStringList&);
        QStringList updateWhitelistFile();
        //void updateSettings(const QList<QTreeWidgetItem*>& treeItems, int firstAddedItemPos=0, bool updateAll=true);
        QString translateConnection(const QString&);
        QStringList getDirections(const QString& chain);
        QStringList getCommands(QStringList items, QStringList connections, QStringList protocols, QList<bool> allows);
        //void addTreeWidgetItemToWhitelist(QTreeWidgetItem* item);
        void load();
        QStringList generateIptablesCommands();
        //QStringList updateWhitelistItemsInIptables(QList<QTreeWidgetItem*> items, GuiOptions *guiOptions);
        bool isPortAdded(const QString& value, const QString & portRange);
        //bool isInPglcmd(const QString& value, const QString& connectType, const QString& prot);
        bool contains(const QString&, const QString&, const QString&);
        bool contains(const WhitelistItem&);
        bool isValid(const QString&, const QString&, const QString&, QString&);
        bool isValid(const WhitelistItem&, QString&);
        WhitelistItem* item(const WhitelistItem&);
        QString getIptablesTestCommand(const QString& value, const QString& connectType, const QString& prot);
        QString parseProtocol(const QString&);
        QString parseConnectionType(const QString&);
        WhitelistItem* whitelistItemAt(int);
        void addItem(WhitelistItem*);
        void removeItemAt(int);
        void updatePglSettings();
        void updateGuiSettings();
        bool isChanged();
        void undo();
        void loadSystemPorts();
        QList<Port> systemPorts();
        QHash<QString, int> systemPortsNameToNumber();
        Port parsePort(QString);
        int portNumber(const QString&);
        bool isPort(const QString&);
};

#endif
