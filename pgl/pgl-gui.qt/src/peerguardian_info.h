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

#ifndef PEERGUARDIAN_INFO_H
#define PEERGUARDIAN_INFO_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QTimer>
#include <QProcess>
#include <QtDebug>

#include "file_transactions.h"
#include "peerguardian_log.h"

#define DAEMON "pgld"
#define EMIT_SIGNAL_DELAY 20000

typedef enum {BLOCKCONTROL_LOG_FILENAME_PLACE,
    MOBLOCK_LOG_FILENAME_PLACE,
	MAX_FILENAME_VECTOR_SIZE
};
	

/**
*
* @short Simple class which loads data from the blockcontrol log and moblock's main log to display information about moblock. This class is also used to check the state of the moblock daemon.
* 
*/

class PeerguardianInfo : public QObject {

	Q_OBJECT	

	public:
		/**
		 * Constructor. Creates a PeerguardianInfo object and loads data from the moblock log in the path specified
		 * @param filename The path to the blockcontrol log
		 * @param parent The QObject parent.
		 */
		PeerguardianInfo( const QString &logFileName, QObject *parent = 0 );
		/**
		 * Constructor. Creates a PeerguardianInfo object but does not load any data.
		 * The data have to be loaded using setFilePath().
		 * However, this class will still be able to check the state of the daemon with no data loaded.
		 * @param parent The QObject parent.
		 */
		PeerguardianInfo( QObject *parent = 0 );
		/**
		 * Destructor.
		 */
		~PeerguardianInfo() { }
		/**
		 * Try to get the information from the file in the specified path.
		 * If the path is invalid and no path is already set, MOBLOCK_INFO_PATH is used.
		 * @param filename The path to the blockcontrol log.
		 */
		void setFilePath( const QString &filename, const int &place );
		/**
		 * @return The number of the ranges moblock has loaded.
		 */
		inline QString loadedRanges() const { return m_LoadedRanges; }	
		/**
		 * @return The number of the ranges moblock has skipped.
		 */
		inline QString skippedRanges() const { return m_SkippedRanges; }
		/**
		 * @return The number of the ranges moblock has merged.
		 */
		inline QString mergedRanges() const { return m_MergedRanges; }
		/**
		 * Get the date/time of the last update of the blocklists.
		 * @return 
		 */
		inline QString lastUpdateTime() const { return m_LastUpdateTime; }
		/**
		 * The state of the moblock daemon.
		 * @return True if moblock is running, otherwise false.
		 */
		inline bool daemonState() const { return m_DaemonState; }
		/**
		 * The process PID of the moblock daemon.
		 * @return The process PID, or a null string if moblock is not running.
		 */
		inline QString processPID() const { return m_ProcessPID; }
		/**
		 * The blockcontrol log.
		 * @return The contents of the blockcontrol log with slightly changed format.
		 */
		inline QVector< QString > controlLog() const { return m_LastUpdateLog; }

	public slots:
		/**
		 * Update the information from blockcontrol log.
		 */
		void updateControlLog();
		/**
		 * Update the daemon state.
		 */
		void updateDaemonState();
		/**
		 * Update the information from moblock's log.
		 */
		void updateLog();
		/**
		 * Called when the data from the moblock log needs to be updated, with a delay of EMIT_SIGNAL_DELAY seconds.
		 */
		void delayedUpdateLog();
	
	signals:
		/**
		 * Emited when the state of the moblock daemon changes.
		 * @param state True if moblock is running, otherwise false.
		 */
		void processStateChange( const bool &state );
		/**
		 * Emited when the PID of the moblock daemon changes.
		 * @param PID The process PID of the moblock daemon.
		 */
		void processPIDChange( const QString &PID );
		/**
		 * Emited when moblock starts running.
		 */
		void moblockStarted();
		/**
		 * Emited when moblock is terminated.
		 */
		void moblockStopped();
		/**
		 * Emited when there is a change in the blockcontrol log.
		 */
		void logChanged();
        

	
	private:	
		//Check if moblock is running
		void checkProcess();
		void updateLogData();
		void updateCLogData();
		void processDate( QString &date );
		QString m_LoadedRanges;
		QString m_SkippedRanges;
		QString m_MergedRanges;
		QString m_LastUpdateTime;
		QString m_ProcessPID;
		bool m_DaemonState;
		bool m_infoLoaded;
		QVector< QString > m_FileNames;
		QVector< QString > m_LastUpdateLog;
		QVector< QString > m_PreviousUpdateLog;
		QVector< QString > m_ControlLogNewL;
};

#endif

