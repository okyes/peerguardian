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

#include "peerguardian_info.h"
#include "utils.h"

PeerguardianInfo::PeerguardianInfo( const QString &logFileName, QObject *parent ) :
	QObject( parent )
{
	
	m_DaemonState = false;
	m_FileNames.resize( MAX_FILENAME_VECTOR_SIZE );
	setFilePath( logFileName, MOBLOCK_LOG_FILENAME_PLACE );
	checkProcess();
}


PeerguardianInfo::PeerguardianInfo( QObject *parent ) :
	QObject( parent )
{

	m_FileNames.resize( MAX_FILENAME_VECTOR_SIZE );
	m_DaemonState = false;

}

void PeerguardianInfo::setFilePath( const QString &filename, const int &place ) {

	if ( filename.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Can't set/change info file at place" << place << "as a null string was given.";
		return;
	}
	else if ( place >= m_FileNames.size() ) {
		qWarning() << Q_FUNC_INFO << "Can't set/change info file because place" << place << "is greater than the vector's size";
		return;
	}

	m_FileNames[place] = filename;

	
	if ( place == BLOCKCONTROL_LOG_FILENAME_PLACE ) {
		updateCLogData();
	}
	else if ( place == MOBLOCK_LOG_FILENAME_PLACE ) {
		updateLogData();
	}


}

void PeerguardianInfo::checkProcess() {

	QString command = "pidof";
	QString output;
	QString moblockProcess;

	QProcess ps;
	ps.start( command, QStringList() << DAEMON );
	if ( ! ps.waitForStarted() ) {
		qWarning() << Q_FUNC_INFO << "Could not get process status for moblock.";
		m_DaemonState = false;
	}
	ps.closeWriteChannel();
	ps.waitForFinished();


	m_ProcessPID = ps.readAll().trimmed();
	m_DaemonState = !m_ProcessPID.isEmpty();

}

void PeerguardianInfo::updateControlLog() {

	updateCLogData();

	if ( m_LastUpdateLog != m_PreviousUpdateLog ) {
		emit logChanged();
		m_PreviousUpdateLog = m_LastUpdateLog;
	}

}

void PeerguardianInfo::updateLog() {

	updateLogData();

}

void PeerguardianInfo::delayedUpdateLog() {

	QTimer::singleShot( EMIT_SIGNAL_DELAY, this, SLOT( updateLog() ) );

}

void PeerguardianInfo::updateDaemonState() {


	QString oldPID = m_ProcessPID;
	bool oldState = m_DaemonState;

	checkProcess();
	

	if ( m_ProcessPID != oldPID ) {
		emit processPIDChange( m_ProcessPID );
	}
	if ( m_DaemonState != oldState ) {
		emit processStateChange( m_DaemonState );
		if ( m_DaemonState == true ) {
			emit moblockStarted();
		}
		else {
			emit moblockStopped();
		}
	}


}


void PeerguardianInfo::updateCLogData() {


	QVector< QString > fileData = getFileData( m_FileNames[BLOCKCONTROL_LOG_FILENAME_PLACE] );

	m_LastUpdateLog = QVector<QString>();
	bool inLog = false;
	QVector< QString > timeDateV(2);
	m_LastUpdateTime = QString();

	if ( fileData.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Failed to retreive information for moblock from file" << m_FileNames[BLOCKCONTROL_LOG_FILENAME_PLACE];
		return;
	}


	for ( int i = 0; i < fileData.size(); i++ ) {

		QString infoStr = fileData[i].trimmed();
		if ( infoStr.isEmpty() ) {
			continue;
		}
		else if ( infoStr.contains( "Begin:" ) ) {
			inLog = true;
			timeDateV = QVector<QString>::fromList( infoStr.split( " " ) );
			processDate( timeDateV[0] );
			QString timeDate = tr( "Operation started: %1 - %2" ).arg(timeDateV[0]).arg(timeDateV[1]);
			m_LastUpdateLog.push_back( timeDate );
		}
		else if ( infoStr.contains( "End:" ) ) {
			inLog = false;
			timeDateV = QVector<QString>::fromList( infoStr.split( " " ) );
			processDate( timeDateV[0] );
			QString timeDate = tr( "Operation finished: %1 - %2" ).arg(timeDateV[0]).arg(timeDateV[1]);
			m_LastUpdateLog.push_back( timeDate );
		}
		else if ( inLog == true ) {

			m_LastUpdateLog.push_back( infoStr );
			if ( infoStr.contains( "Blocklists updated", Qt::CaseInsensitive ) ) {
				if ( !timeDateV[0].isNull() && !timeDateV[1].isNull() ) {
					m_LastUpdateTime = tr( "%1 - %2" ).arg(timeDateV[0]).arg(timeDateV[1]);
				}
				else {
					m_LastUpdateTime = QString();
				}
			}
		}

	}
}

void PeerguardianInfo::updateLogData() {

	QVector< QString > fileData = getFileData( m_FileNames[MOBLOCK_LOG_FILENAME_PLACE] );
	m_LoadedRanges = QString();
	m_SkippedRanges = QString();
	m_MergedRanges = QString();

	if ( fileData.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Failed to retreive information for moblock from file" << m_FileNames[MOBLOCK_LOG_FILENAME_PLACE];
		return;
	}

	for ( int i = fileData.size() - 1; i >= 0; i--) {
	
		if ( fileData[i].startsWith( "*" ) ) {
			fileData[i].remove( "*" );
		}
		fileData[i] = fileData[i].trimmed();
		if ( fileData[i].startsWith( "Ranges loaded:", Qt::CaseInsensitive ) ) {
			m_LoadedRanges = fileData[i].section( ":", 1, 1, QString::SectionSkipEmpty ).trimmed();
			break;
		}
	}

}

void PeerguardianInfo::processDate( QString &date ) {

	static QString prevDate;
	static QString prevResult;
	
	//Calculate the result only one time if the date is the same
	if ( prevDate == date ) {
		date = prevResult;
		return;
	}

	prevDate = date;

	QString year = date.section("-", 0, 0 );
	QString month = date.section( "-", 1, 1 );
	QString day = date.section( "-", 2, 2 );

	month = ( month.startsWith( "0" )  ?  QString( month[1] )  : month );
	day = ( day.startsWith( "0" )  ? QString( day[1] ) : day );

	date = tr( "%1/%2/%3" ).arg(day).arg(month).arg(year);
	prevResult = date;

}


/*** Static methods ***/

QString PeerguardianInfo::getFilePath()
{
    QString path("");
    return getValidPath(path, PGL_LOG_PATH);
}

QString PeerguardianInfo::getFilePath(const QString &path)
{
    return getValidPath(path, PGL_LOG_PATH);
}


