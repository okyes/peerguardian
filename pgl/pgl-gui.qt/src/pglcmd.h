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

#ifndef BLOCKCONTROL_H
#define BLOCKCONTROL_H

#include <QObject>
#include <QProcess>
#include <QtDebug>
#include <QFile>
#include <QString>
#include <QStringList>

#include "super_user.h"

#define PGLCMD_PATH "/usr/bin/pglcmd"

#define MESSAGE_TIMEOUT 20000

/**
*
* @short Simple class using SuperUser to handle the moblock daemon
*
*/


class PglCmd : public SuperUser {

	Q_OBJECT


	public:
		/**
		 * Default constructor, creates a blockcontrol object using the script specified in path.
		 * @param path The path of the blockcontrol script.
		 * @param parent The parent of this QObject.
		 */
		PglCmd( const QString &path, QObject *parent = 0 );
		/**
		 * Constructor, creates a blockcontrol object without setting any path for the blockcontrol script.
		 * @param parent The parent of this QObject.
		 */
		PglCmd( QObject *parent = 0 );
		/**
		 * Destructor
		 */
		virtual ~PglCmd() { };
		/**
		 * Set the path to the blockcontrol script.
		 * If the path specified is empty, then the default path, PGLCMD_PATH is used instead.
		 * @param path The path to the blockcontrol script file.
		 */
		void setFilePath( const QString &path, bool verified=false );
        
        QString getPath();
        static QString getFilePath();
        static QString getFilePath(const QString &path);

	public slots:
		/**
		 * Start the moblock daemon using blockcontrol
		 */
		void start();
		/**
		 * Restart the moblock daemon using blockcontrol
		 */
		void restart();
		/**
		 * Stop the moblock daemon using blockcontrol
		 */
		void stop();
		/**
		 * Reload moblock using blockcontrol
		 */
		void reload();
		/**
		 * Update moblock using blockcontrol
		 */
		void update();
		/**
		 * Get the blockcontrol status.
		 */
		void status();
        

	signals:
		/**
		 * Message describing the currently running action
		 * @param  message The message describing the action
		 * @param timeout The time the message will be displayed
		 */
		void actionMessage( const QString &message, const int &timeout );
	
	private:
		QString m_FileName;

};

#endif
