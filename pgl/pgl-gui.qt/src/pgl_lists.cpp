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


ListItem::ListItem( const QString &itemRawLine ) {

    QString itemLine = itemRawLine.trimmed();
    
    option = NONE;
    m_Location = "";
    
    if ( itemLine.isEmpty() ) {
        mode = COMMENT_ITEM;
    }
    else if (itemLine.startsWith('#'))
    {
        if ( isValidBlockList(itemLine) )
        {
            mode = DISABLED_ITEM;
            m_Name = getListName(itemLine);
        }
        else
            mode = COMMENT_ITEM;
    }
    else if( isValidBlockList(itemLine) )
    {
        mode = ENABLED_ITEM;
        m_Name = getListName(itemLine);
    }

}

bool ListItem::isValidBlockList(const QString & line)
{
    QStringList formats;
    formats << "7z" << "dat" << "gz" << "p2p" << "zip";
    
    if ( line.contains("list.iblocklist.com") )
        return true;
    
    if ( QFile::exists(line) )
        foreach(QString format, formats)
            if ( line.endsWith(format) )
                return true;

    return false;
}

QString ListItem::getListName(const QString& line)
{
    if ( line.contains("/") )
        return line.split("/").last();
    else
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

QString ListItem::exportItem() const {
    
    QString finalOut;
            
    if ( mode != ENABLED_ITEM ) {
        finalOut += "#";
    }
    if ( option == NO_TIME_STAMP ) {
        finalOut += "notimestamp ";
    }
    else if ( option == LOCAL ) {
        finalOut += "locallist ";
    }

    finalOut += m_Location;
    return finalOut;

}


PeerguardianList::PeerguardianList( const QString &path ) 
{

    setFilePath(path, true);
    
}

QString PeerguardianList::getListPath()
{
    return m_FileName;
}


void PeerguardianList::setFilePath( const QString &path, bool verified ) {

    if ( verified )
        m_FileName = path;
    else
        m_FileName = getValidPath(path, PGL_LIST_PATH);
    
    if ( m_FileName.isEmpty() ){
        qWarning() << Q_FUNC_INFO << "Empty path given, doing nothing";
        return;
    }
        
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

void PeerguardianList::addItem( const QString &location, listOption opt ) {

    ListItem newItem( location );

    newItem.option = opt;

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

QVector< ListItem * > PeerguardianList::getValidItems() {
    
    QVector< ListItem * > result;
    for ( QVector< ListItem >::iterator s = m_ListsFile.begin(); s != m_ListsFile.end(); s++ ) 
        if ( s->mode != COMMENT_ITEM ) 
            result.push_back(s);
        
    
    return result;
}
    
bool PeerguardianList::exportToFile( const QString &filename ) {

    QStringList tempFile;
    for ( QVector< ListItem >::const_iterator s = m_ListsFile.begin(); s != m_ListsFile.end(); s++ ) 
        tempFile.push_back( s->exportItem() );

    return saveFileData( tempFile, filename );

}


QVector< ListItem * >  PeerguardianList::getEnabledItems()
{   
    return getItems(ENABLED_ITEM);
}

QVector< ListItem * >  PeerguardianList::getDisabledItems()
{
    return getItems(DISABLED_ITEM);
}

/*** Static methods ***/

QString PeerguardianList::getFilePath()
{
    QString path("");
    return getValidPath(path, PGL_LIST_PATH);
}

QString PeerguardianList::getFilePath(const QString &path)
{
    return getValidPath(path, PGL_LIST_PATH);
}


