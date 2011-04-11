/***************************************************************************
 *   Copyright (C) 2007-2008 by Dimitris Palyvos-Giannas   *
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
#include <QFile>

#include "peerguardian_log.h"
#include "utils.h"
#include "pgl_settings.h"

LogItem::LogItem( const QString &entry ) {

	QString entryContents;
	QString entryTimeStamp;

	if ( entry.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Emtpy log item entry given. Doing nothing";	
	}
	else 
    {
        if ( entry.contains("||") )
        {
            QStringList entryParts(entry.split( " ", QString::SkipEmptyParts) );
            QStringList names(entry.split( "||", QString::SkipEmptyParts) );
            m_Name = names[1];
            
            m_BlockTime = entryParts[2].trimmed();
            m_ipSource = entryParts[4];
            m_ipDest = entryParts[5];
            m_Hits = 1;
            return;
        }
    }
        
        type = IGNORE;
        /*
		QVector< QString > entryParts = QVector< QString >::fromList( entry.split( "|" ) );
		if ( entryParts.size() == 1 ) {
			entryContents = entryParts[0];
			initDateTime();
		}
		else if ( entryParts.size() == 2 ) {
			entryContents = entryParts[1];
			initDateTime();
		}
		else if ( entryParts.size() > 2 ) {
			for ( int i = 1; i < entryParts.size(); i++ ) {
				entryContents += " " + entryParts[i];
			}
			initDateTime();
		}
		entryContents = entryContents.trimmed();
	}
		
	if ( entryContents.startsWith( "Ski", Qt::CaseInsensitive ) || entryContents.startsWith( "Ignore", Qt::CaseInsensitive ) || entryContents.startsWith( "Dupli", Qt::CaseInsensitive ) || entryContents.startsWith( "Merge", Qt::CaseInsensitive )) {
		type = IGNORE;
	}
	else if ( entryContents.startsWith( "Error" , Qt::CaseInsensitive ) ) {
		type = ERROR;
		m_Name = entryContents;
	}
	else if ( entryContents.contains( "hits" ) ) {
		importEntry( entryContents );
	}
	else if ( entryContents.contains( "Error", Qt::CaseInsensitive ) || entryContents.contains( "fatal", Qt::CaseInsensitive ) ){
		type = ERROR;
		m_Name = entryContents;
	}
	else {
		type = IGNORE;
		m_Name = entryContents;
	}*/

}

void LogItem::initDateTime( const QString &dateTime ) {

	if ( !dateTime.isEmpty() ) {
		m_BlockTime = dateTime.section( " ", 0, 2 );
		m_BlockTime = dateTime.section( " ", 3, 3 );
	}	

	//Set the date and the time if it is not already set
	m_BlockDate = ( m_BlockDate.isNull() ? QDate( QDate::currentDate() ).toString() : m_BlockDate );
	m_BlockTime = ( m_BlockTime.isNull() ? QTime( QTime::currentTime() ).toString() : m_BlockTime );

}

void LogItem::importEntry( const QString &entry ) {

	
	static const QString IN( "IN:");
	static const QString OUT( "OUT:" );
	static const QString FWD( "FWD:" );
	
	int nameStart = 0;
	int namePlace = 0;
	int removeNameEnd = 0;
	//"Hits:" length = 5
	static const int removeHitsEnd = 5;
	//"SRC:" / "DST:" length = 4
	static const int removeIPEnd = 4;

	QString entryPartName;
	QString entryPartHits;
	QString entryPartIP;
	//Split the string and save the parts into a vector
	QVector< QString > entryVector = QVector< QString >::fromList( entry.split( ",", QString::SkipEmptyParts ) );

	//namePlace variable is used to avoid wrong input on entries like
	//Blocked OUT: INTERNET SOFTWARE CONSORTIUM, INC,hits: 22,DST: 204.152.184.64
	namePlace = entryVector.size() - 3;
	//Name
	for ( int i = nameStart; i <= namePlace; i++ ) 
		entryPartName += entryVector[i];
	//Hits
	entryPartHits = entryVector[namePlace + 1];
	//IP
	entryPartIP = entryVector[namePlace + 2];
	//Process the first part of the string to determine the item's type
	if ( entryPartName.contains( OUT, Qt::CaseInsensitive ) ) {
		removeNameEnd = entryPartName.indexOf( ":" );
		type = BLOCK_OUT;
		
	}
	else if ( entryPartName.contains( IN , Qt::CaseInsensitive ) ) {
		removeNameEnd = entryPartName.indexOf( ":" );
		type = BLOCK_IN;
	}
	else if ( entryPartName.contains( FWD , Qt::CaseInsensitive ) ) {
		removeNameEnd = entryPartName.indexOf( ":" );
		type = BLOCK_FWD;
	}
	//This sould normally never happen, but if it does...
	else {
		if ( entry.contains( "Error", Qt::CaseInsensitive ) ) {
			type = ERROR;
		}
		else {
			type = IGNORE;
		}
		m_Name = entry;
		return;
	}
	//Process the first part of the string, removing everything before the :(timestamp and block/mark message)
	entryPartName.remove( 0, removeNameEnd + 1 );
	//Process the second part of the string, removing everything before the :
	//E.g. hits: 20
	//removeHitsEnd = entryPartHits.lastIndexOf( ":" );
	entryPartHits.remove( 0, removeHitsEnd + 1 );
	//Process the third part of the string, the IP, removing everything before the :
	//removeIPEnd = entryPartIP.lastIndexOf( ":" );
	entryPartIP.remove( 0, removeIPEnd + 1 );
	//Set the variables
	m_Name = entryPartName.trimmed();
	m_Hits = entryPartHits.trimmed();
	//m_IP = entryPartIP.trimmed();

}


bool LogItem::operator==( const LogItem &other ) {

	bool equal = true;
    
	if ( m_Name != other.name() ) 
		equal = false;
	
	if ( type != other.type ) 
		equal = false;
	
	if ( m_ipSource != other.getIpSource() ) 
		equal = false;
	
    if ( m_ipDest != other.getIpDest() ) 
		equal = false;
	

	return equal;

}

PeerguardianLog::PeerguardianLog(QObject *parent ) :
	QObject( parent )
{
}


void PeerguardianLog::setFilePath( const QString &path, bool verified) {

	//Empty the list and intiallize the variables
	clear();
    
    if ( verified )
         m_LogPath = path;
    else{
        m_LogPath = PeerguardianLog::getFilePath(path);
        if ( m_LogPath.isEmpty() )
            qWarning() << Q_FUNC_INFO << "PeerguardianLog: No valid path was found, doing nothing";
    }
}

QString PeerguardianLog::getLogPath()
{
	return m_LogPath;
}


void PeerguardianLog::clear() {

	m_ItemsList = QList< LogItem >();
	m_ItemsList.push_back( LogItem() );
	firstTime = true;
	firstItem = QString();
}


QString PeerguardianLog::getNewItem() const {


	if ( m_LogPath.isEmpty() )
		return QString();
	
	QString command;
	QString error;
	QString entry;
	QStringList args;
	
	//Use tail /var/log/pgl/pgld.log -n 1 to get the last log entry
	command = "tail";
	args << m_LogPath;
	args << "-n" << "1";
	//Execute tail and get output
	QProcess tail;
	tail.start( command, args );
	tail.waitForStarted();
	tail.closeWriteChannel();
	tail.waitForFinished();
	entry = tail.readAll().trimmed();
	
	if ( entry.isEmpty() ) {
		//qWarning() << Q_FUNC_INFO << "Failed to retreive data from" << m_LogPath;
		return QString();
	}

	return entry;

}

void PeerguardianLog::update() {


	QString lItem = getNewItem();
	if ( lItem.isEmpty() ) { 
		return;
	}
	//We don't want the last item in the list to appear as a recently blocked entry
	if ( firstTime ) {
		firstItem = lItem;
		firstTime =  false;
	}
	if ( firstItem == lItem ) {
        qDebug() << lItem;
        //qDebug() << "equal items";
		return;
	}

	LogItem freshItem( lItem );

	if ( freshItem.type == IGNORE  ) {
        qDebug() << "ignore items";
		return;
	}
	LogItem prevItem = m_ItemsList.last();

	if ( freshItem == prevItem ) {
		//The two items are the same, no need to update
		if ( freshItem.hits() == prevItem.hits() ) {
			return;
		}
		//Else emit the new hits string
		else {
			emit newItemHits( freshItem.hits() );
			emit newItem( freshItem );
			m_ItemsList.pop_back();
			m_ItemsList.append( freshItem );
		} 

	}
	//If the item has changed emit all the new values
	else {
		emit newItem( freshItem );
		emit newItemHits( freshItem.hits() );
		
		//Insert the item in the list
		//If another occurence of the item is already in the list remove it
		int place = m_ItemsList.indexOf( freshItem );
		if ( place == -1 ) {
			m_ItemsList.append( freshItem );
		}
		else {
			m_ItemsList.removeAt( place );
			m_ItemsList.append( freshItem );
		}
		
	}
	//If the list is too big remove the first item
	if ( m_ItemsList.size() > MAX_LOG_SIZE - 1 ) {
		qDebug() << Q_FUNC_INFO << "Log items list is full, removing" << m_ItemsList.first().name();
		m_ItemsList.pop_front();
	}

}

LogItem PeerguardianLog::getItemByIP( const QString &IP, const itemType &type ) const {

	LogItem result;
	//In case nothing is found ignore this item
	result.type = IGNORE;

	for ( QList< LogItem >::const_iterator s = m_ItemsList.constBegin(); s != m_ItemsList.constEnd(); s++ ) {
		if ( s->getIpSource() == IP && s->type == type ) {
			result = *s;
		}
	}

	return result;
}	

LogItem PeerguardianLog::getItemByName( const QString &name ) const {

	LogItem result;
	//In case nothing is found ignore this item
	result.type = IGNORE;


	for ( QList< LogItem >::const_iterator s = m_ItemsList.constBegin(); s != m_ItemsList.constEnd(); s++ ) {
		if ( s->name() == name ) {
			result = *s;
		}
	}
	return result;
}



/*** Static methods ***/

QString PeerguardianLog::getFilePath()
{
    QString path("");
    return getValidPath(path, getValue(PGLCMD_DEFAULTS_PATH, "LOGDIR"));
}

QString PeerguardianLog::getFilePath(const QString &path)
{
    return getValidPath(path, getValue(PGLCMD_DEFAULTS_PATH, "LOGDIR"));
}
