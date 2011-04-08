 
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


#ifndef SUPER_USER_H
#define SUPER_USER_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>

#include "proc_thread.h"

#define KDESU_PATH "/usr/bin/kdesudo"
#define GKSU_PATH "/usr/bin/gksu"
#define KDESU "kdesu"
#define GKSU "gksu"

/**
*
* @short A simple class to execute commands with sudo rights.
*
*/


class SuperUser : public QObject
{
    
    Q_OBJECT
    
	public:
		/**
		 * Constructor. 
		 */
        
		SuperUser(QObject *parent = 0 );
        SuperUser(QString& rootpath, QObject *parent = 0 );
		/**
		 * Destructor.
		 */
		virtual ~SuperUser();
		/**
		 * Set the path to the sudo front end the class is going to use.
		 * @param path The new path.
		 */
		void setFilePath( const QString &path );
		/**
		 * Execute a command with sudo rights.
		 * @param command QStringList containing the command to be executed.
		 * @param detached Execute the command either detached or start it normally and wait for it to be finished. This is ingored if the mobloquer was started with sudo rights.
		 */
		void execute( const QStringList &command );
		/**
		 * Move a file from one place of the filesystem to another, using the mv command.
		 * @param source The current path of the file.
		 * @param dest The file's new location.
		 */
		void moveFile( const QString &source, const QString &dest );
		/**
		 * Copy a file from one place of the filesystem to another, using the cp command.
		 * @param source The current path of the file.
		 * @param dest The file's new location.
		 */
		void copyFile( const QString &source, const QString &dest );
		/**
		 * Remove a file using the mv command.
		 * @param source The current path of the file.
		 */
		void removeFile( const QString &source );
        
        void setRootPath(QString& path, bool verified=false);
        QString getRootPath();
        static QString getFilePath();
        static QString getFilePath(const QString &path);
        void startThread(const QString &name, const QStringList &args, const QProcess::ProcessChannelMode &mode );
        void moveFiles( const QMap<QString, QString>);
        void operator=(const SuperUser& su);
        
    public slots:
        void processFinished();
        
    signals:
        void finished();
        
	private:
        SuperUser(const SuperUser& other);
		static QString m_SudoCmd;
		static QString m_TestFile;
        QList<ProcessT*> m_threads;
        QObject *m_parent; 
        QMap<QString, QString> m_filesToMove;

};

#endif
