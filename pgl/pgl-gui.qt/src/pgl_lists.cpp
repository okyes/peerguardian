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
	
	mode = ENABLED_ITEM;
	option = NONE;
	//In order to export it later if it's a comment

	if ( itemLine.isEmpty() ) {
		mode = COMMENT_ITEM;
	}
	//Check if the line is a comment or just a disabled entry
	else if ( itemLine.startsWith( "#" ) ) {
		
		//Assume it's a comment for now
		mode = COMMENT_ITEM;
		itemLine.remove( "#" );
		m_Name = m_Location = itemLine;
		//Check it the end of the line is a valid blocklist file extension
		if ( itemLine.endsWith( ".gz" ) )
			mode = DISABLED_ITEM;
		else if ( itemLine.endsWith( ".zip" ) )
			mode = DISABLED_ITEM;
		else if ( itemLine.endsWith( ".7z" ) )
			mode = DISABLED_ITEM;
		else if ( itemLine.endsWith( ".dat" ) )
			mode = DISABLED_ITEM;
		else if ( itemLine.endsWith( ".p2b" ) )
			mode = DISABLED_ITEM;
		else if ( itemLine.endsWith( ".p2p" ) )
			mode = DISABLED_ITEM;
	}
	if ( mode != COMMENT_ITEM ) {

		QVector<QString > spaceSplit = QVector< QString >::fromList( itemLine.split(" ") );
		//One more check for comments/invalid entries, just in case
		if ( spaceSplit.size() > 2 ) {
			qWarning() << Q_FUNC_INFO << "Ignoring possibly invalid blocklist entry, " << itemRawLine;
			mode = COMMENT_ITEM;
		}
		//Get the item's Option if it there is one
		else if ( spaceSplit.size() == 2 ) {
			
			if ( spaceSplit[0] == "locallist" ) {
				option = LOCAL;
				m_Location = spaceSplit[1].trimmed();
			}
			else if ( spaceSplit[0] == "notimestamp" ) {
				option = NO_TIME_STAMP;
				m_Location = spaceSplit[1].trimmed();
			}
			else {
				qWarning() << Q_FUNC_INFO << "Ignoring possibly invalid blocklist entry, " << itemRawLine;
				mode = COMMENT_ITEM;
			}
		}
		//If the item contains only the location
		else if ( spaceSplit.size() == 1 ) {
			option = NONE;
			m_Location = spaceSplit[0].trimmed();
		}
	}
	//May have changed after the last check
	if ( mode != COMMENT_ITEM ) {
		//Get the filename and its extension from the item's location
		QVector< QString > slashSplit = QVector< QString >::fromList( m_Location.split( "/" ) );
		QVector< QString > adrSplit = QVector< QString >::fromList( slashSplit[ slashSplit.size() - 1 ].split( "." ) );
		m_Name = adrSplit[0];
		for ( int i = 1; i < adrSplit.size(); i++ ) {
			m_Format += adrSplit[i];
		}
		m_Format.remove( m_Format.indexOf( "." ) );

	}

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
    
    if ( m_FileName.isEmpty() )
        qWarning() << Q_FUNC_INFO << "Empty path given, doing nothing";

	m_ListsFile = QVector< ListItem >();

	QVector< QString > tempFileData = getFileData( m_FileName );
	for ( QVector< QString >::const_iterator s = tempFileData.begin(); s != tempFileData.end(); s++ ) {
		ListItem tItem( *s );
		m_ListsFile.push_back( tItem );
	}
    
	//Check for double entries and delete them to prevent strange behaviour
	//If there are two same urls and the one has the notimestamp attribute, we keep this instead of the other
	for ( int i = 0; i < m_ListsFile.size(); i++ ) {
		
		ListItem tempItem = m_ListsFile[i];

		if ( tempItem.mode == COMMENT_ITEM )
			continue;

		int j = m_ListsFile.indexOf( tempItem );
		while ( j != -1 ) {
			if ( tempItem.mode == DISABLED_ITEM && m_ListsFile[j].mode == ENABLED_ITEM ) 
				tempItem = m_ListsFile[j];
			
			if ( m_ListsFile[j].option == NO_TIME_STAMP && m_ListsFile[j].mode == ENABLED_ITEM ) 
				tempItem = m_ListsFile[j];
			
			m_ListsFile.remove( j );	
			j = m_ListsFile.indexOf( tempItem );
		}
		m_ListsFile.insert( i, tempItem );

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

QVector< ListItem * > PeerguardianList::getItems() {
	
	QVector< ListItem * > result;
	for ( QVector< ListItem >::iterator s = m_ListsFile.begin(); s != m_ListsFile.end(); s++ ) 
		if ( s->mode != COMMENT_ITEM ) 
			result.push_back(s);
		
	
	return result;
}
	
bool PeerguardianList::exportToFile( const QString &filename ) {

	QVector< QString > tempFile;
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


