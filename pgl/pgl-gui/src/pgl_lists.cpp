/***************************************************************************
 *   Copyright (C) 2007 by Dimitris Palyvos-Giannas   *
 *   jimaras@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
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

#include "pgl_lists.h"
#include "utils.h"
#include "pgl_settings.h"


ListItem::ListItem( const QString &itemRawLine ) {

    QString itemLine = itemRawLine.trimmed();

    m_Location = "";
    mode = COMMENT_ITEM;

    if ( itemLine.isEmpty() ) {
        mode = COMMENT_ITEM;
    }
    else if (itemLine.startsWith('#'))
    {
        if ( isValidBlockList(itemLine) )
        {
            mode = DISABLED_ITEM;
            m_Name = getListName(itemLine);
            m_Location = itemLine.split("#")[1];
        }
        else
            mode = COMMENT_ITEM;
    }
    else if( isValidBlockList(itemLine) )
    {
        mode = ENABLED_ITEM;
        m_Name = getListName(itemLine);
        m_Location = itemLine;
    }

}

bool ListItem::isValidBlockList(const QString & line)
{
    QStringList formats;
    formats << "7z" << "dat" << "gz" << "p2p" << "zip";

    if ( line.contains("list.iblocklist.com") )
        return true;
        
    if ( line.contains("http://") || line.contains("ftp://") || line.contains("https://") )
        return true;

    if ( QFile::exists(line) )
        foreach(QString format, formats)
            if ( line.endsWith(format) )
                return true;

    return false;
}

QString ListItem::getListName(const QString& line)
{
    if ( line.contains("list.iblocklist.com/lists/") )
        return line.split("list.iblocklist.com/lists/").last();

    QFileInfo file(line);

    if ( file.exists() )
        return file.fileName();

    return line;
}

bool ListItem::isDisabled() {
    if ( mode == DISABLED_ITEM )
        return true;

    return false;
}

bool ListItem::isEnabled() {

    if ( mode == ENABLED_ITEM )
        return true;

    return false;

}

bool ListItem::operator==( const ListItem &other ) {

    if ( location() != other.location() )
        return false;

    return true;

}

PeerguardianList::PeerguardianList( const QString &path )
{

    setFilePath(path, true);
    m_localBlocklistDir = PglSettings::getStoredValue("LOCAL_BLOCKLIST_DIR") + "/";

}

QString PeerguardianList::getListPath()
{
    return m_FileName;
}


void PeerguardianList::setFilePath( const QString &path, bool verified ) {

    if ( verified )
        m_FileName = path;
    else
        m_FileName = getValidPath(path, PglSettings::getStoredValue("BLOCKLISTS_LIST"));

    if ( m_FileName.isEmpty() )
        qWarning() << Q_FUNC_INFO << "Empty path given, doing nothing";

}

void PeerguardianList::updateListsFromFile()
{
    if ( m_FileName.isEmpty() )
        return;

    m_ListsFile = QVector< ListItem >();
    QStringList tempFileData = getFileData( m_FileName );


    for( int i=0; i < tempFileData.size(); i++)
    {
        ListItem tempItem(tempFileData[i]);

        if ( tempItem.mode == COMMENT_ITEM )
            continue;

        m_ListsFile.push_back(tempItem);
    }
}


int PeerguardianList::indexOfName( const QString &name ) {

        for ( int i = 0; i < m_ListsFile.size(); i++ ) {
            if ( m_ListsFile[i].name() == name ) {
                return i;
            }
        }

    return -1;

}




void PeerguardianList::addItem( const ListItem &newItem ) {

    if ( newItem.mode == COMMENT_ITEM ) {

        qWarning() << Q_FUNC_INFO << "Inserting COMMENT_ITEM into the blocklist vector.";

    }
    m_ListsFile.push_back( newItem );


}

void PeerguardianList::addItem( const QString &line ) {

    ListItem newItem( line );

    addItem( newItem );

}

void PeerguardianList::setMode( const ListItem &item, const itemMode &mode ) {

    int i = m_ListsFile.indexOf( item );
    if ( i >= 0 ) {
        m_ListsFile[i].mode = mode;
    }

}

void PeerguardianList::setModeByLocation( const QString &location, const itemMode &mode ) {

    ListItem *item = getItemByLocation( location );
    if ( item != NULL ) {
        setMode( *item, mode );
    }

}

void PeerguardianList::setModeByName( const QString &name, const itemMode &mode ) {

    ListItem *item = getItemByName( name );
    if ( item != NULL ) {
        setMode( *item, mode );
    }

}

void PeerguardianList::removeItem( const ListItem &item ) {

    int i = m_ListsFile.indexOf( item );
    if ( i >= 0 ) {
        m_ListsFile.remove( i );
    }

}

void PeerguardianList::removeItemByLocation( const QString &location ) {

    ListItem *item = getItemByLocation( location );
    if ( item != NULL ) {
        removeItem( *item );
    }

}

void PeerguardianList::removeItemByName( const QString &name) {

    ListItem *item = getItemByName( name );
    if ( item != NULL ) {
        removeItem( *item );
    }


}

ListItem *PeerguardianList::getItemByName( const QString &name  ) {


    for ( QVector< ListItem >::iterator s = m_ListsFile.begin(); s != m_ListsFile.end(); s++ ) {
        if ( s->name() == name )
            return s;
    }


    return NULL;

}

ListItem *PeerguardianList::getItemByLocation( const QString &location ) {


    for ( QVector< ListItem >::iterator s = m_ListsFile.begin(); s != m_ListsFile.end(); s++ ) {
        if ( s->location() == location )
            return s;
    }

    return NULL;
}

QVector< ListItem *> PeerguardianList::getItems( const itemMode &mode ) {

    QVector< ListItem * > result;

    for ( QVector< ListItem >::iterator s = m_ListsFile.begin(); s != m_ListsFile.end(); s++ )
        if ( s->mode == mode )
            result.push_back(s);


    return result;

}

QVector< ListItem * > PeerguardianList::getValidItems()
{
    QVector< ListItem * > result;
    for ( QVector< ListItem >::iterator s = m_ListsFile.begin(); s != m_ListsFile.end(); s++ )
        if ( s->mode != COMMENT_ITEM )
            result.push_back(s);


    return result;
}

QFileInfoList PeerguardianList::getLocalBlocklists()
{
    QFileInfoList localBlocklists;
    QDir defaultDir (m_localBlocklistDir);

    foreach(QFileInfo fileInfo, defaultDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files) )
        if ( fileInfo.isSymLink() )
            localBlocklists << QFileInfo(fileInfo.symLinkTarget());

    return localBlocklists;
}



QVector< ListItem * >  PeerguardianList::getEnabledItems()
{
    return getItems(ENABLED_ITEM);
}

QVector< ListItem * >  PeerguardianList::getDisabledItems()
{
    return getItems(DISABLED_ITEM);
}

void PeerguardianList::update(QList<QTreeWidgetItem*> treeItems)
{

    QStringList fileData = getFileData(m_FileName);
    QStringList newFileData;
    QString line;
    bool state;

    m_localLists.clear();

    //load comments to newFileData
    foreach(QString line, fileData)
    {
        ListItem listItem = ListItem(line);

        if ( listItem.mode == COMMENT_ITEM )
            newFileData << line;
    }

    //load lists to newFileData
    foreach(QTreeWidgetItem * treeItem, treeItems)
    {
        //if it's a filepath
        if ( QFile::exists( treeItem->text(1) ) )
        {
            state = true;
            if ( treeItem->checkState(0) == Qt::Unchecked )
                state = false;

            m_localLists.insert(treeItem->text(1), state);

        }
        else //if it's an URL
        {
            if ( treeItem->checkState(0) == Qt::Unchecked )
                line = "# ";

            line += treeItem->text(1).trimmed();

            newFileData << line;

            line.clear();
        }
    }

    QString filepath = "/tmp/" + getFileName(m_FileName);
    saveFileData(newFileData, filepath);
}

/*** Static methods ***/

QString PeerguardianList::getFilePath()
{
    QString path("");
    return getValidPath(path, PglSettings::getStoredValue("BLOCKLISTS_LIST"));
}

QString PeerguardianList::getFilePath(const QString &path)
{
    return getValidPath(path, PglSettings::getStoredValue("BLOCKLISTS_LIST"));
}


