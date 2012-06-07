
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
#include <QString>
#include <QDebug>
#include <QSettings>
#include <QTimer>

#include "proc_thread.h"

#define PREFIX1 "/usr/bin/"
#define PREFIX2 "/usr/local/bin/"
#define PREFIX3 "$HOME/.local/bin/"
#define KDESU "kdesu"
#define KDESUDO "kdesudo"
#define GKSU "gksu"
#define GKSUDO "gksudo"
#define TMP_SCRIPT "/tmp/execute-all-pgl-commands.sh"

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
        SuperUser(QObject *parent = 0, const QString& rootpath ="");
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
            * @param detached Execute the command either detached or start it normally and wait for it to be finished. This is ingored if pglgui was started with sudo rights.
            */
        void executeCommands( QStringList commands, bool start = true );
        void execute( const QStringList& command);
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

        QString getRootPath();
        //static QString getFilePath();
        //static QString getFilePath(const QString &path);
        //static QString getGraphicalSudoPath();
        static QString sudoCommand();
        static void setSudoCommand(const QString&);
        void startThread(const QString &name, const QStringList &args, const QProcess::ProcessChannelMode &mode );
        void moveFiles( const QMap<QString, QString>, bool start = true);
        void executeScript();
        void operator=(const SuperUser& su);
        void executeCommand(QString& command, bool start = true);
        void executeCommand(QStringList& command, bool start = true);

    public slots:
        void processFinished(const CommandList&);

    signals:
        void finished();
        void error(const CommandList&);
        void error(const QString&);

    private:
        SuperUser(const SuperUser& other);
        void findGraphicalSudo();
        QMap<QString, QString> m_filesToMove;
        ProcessT * m_ProcT;
        QStringList m_Commands;
        bool emitFinished;
        bool mGetSudoCommand;

};

#endif
