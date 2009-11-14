/**************************************************************************
*   Copyright (C) 2009-2010 by Dimitris Palyvos-Giannas                   *
*   jimaras@gmail.com                                                     *
*                                                                         *
*   This is a part of pgl-gui.                                            *
*   Pgl-gui is free software; you can redistribute it and/or modify       *
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

#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include <QObject>
#include <QThread>
#include <QProcess>

#include <QString>

/**
 * @brief A class used to handle processes.
 * 
 * ProcessHandler creates a new QProcess and runs it. 
 * The main program does not have to wait for the process to finish its execution.
 * Moreover, the class saves and gives the output of the process when it's requested.
 */

class ProcessHandler : public QThread {

    Q_OBJECT

    public:
        /**
         * Default constructor.
         * 
         * Creates an empty object.
         * @param parent The parent of this object.
         */
        ProcessHandler( QObject *parent = 0 );
        /**
         * Destructor
         */
        ~ProcessHandler();
        /**
        * Set the given process to be executed when run() is called.
        * @param cmd The name of the process including its arguments.
        * @param mode The process channel modes of the command which will be executed.
        */
        void setCommand( const QString &cmd, const QProcess::ProcessChannelMode &mode = QProcess::SeparateChannels );
        /**
        * Execute the given command. Calls setCommand() first and then just starts the thread if it's not running.
        * @param cmd The name of the process including its arguments.
        * @param mode The process channel modes of the command which will be executed.
        */
        void runCommand( const QString &cmd, const QProcess::ProcessChannelMode &mode = QProcess::SeparateChannels );
        
    public slots:
        /**
        * Reimplementation of QThread::run().
        * Executes the command which was set using setCommand().
        * If no command was set, this function does nothing.
        */
        void run();

    signals:
        /**
        * Emitted when a command has finished running.
        * @param output The output of the command which was executed.
        */
        void processDataS( QString output );
        

    private:
        QString m_Cmd;
        QStringList m_Args;
        QProcess::ProcessChannelMode m_ChanMode;

};

#endif //PROCESSHANDLER_H