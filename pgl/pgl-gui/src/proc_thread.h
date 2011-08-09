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


#ifndef PROC_THREAD_H
#define PROC_THREAD_H

#include <QObject>
#include <QThread>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QtDebug>
#include <QList>
#include <QTimer>


/**
*
*
* @short Class which can execute a command as a seperate thread.
*
*/

class ProcessT : public QThread {

	Q_OBJECT

	public:
		/**
		 * Constructor. Creats a new ProcessT object.
		 * @param parent The QObject parent of this object.
		 */
		ProcessT( QObject *parent = 0 );
        ProcessT(ProcessT const& other) { *this = other; }
		/**
		 * Destructor.
		 */
		virtual ~ProcessT();
		/**
		 * Reimplementation of QThread::run().
		 * Executes the command which was set using setCommand().
		 * If no command was set, this function does nothing.
		 */
		void run();
		/**
		 * Set the given command to be executed when run() is called.
		 * @param name The name of the program.
		 * @param args The command line arguments.
		 * @param mode The process channel modes of the command which will be executed.
		 */
		void setCommand( const QString &name, const QStringList &args, const QProcess::ProcessChannelMode &mode = QProcess::SeparateChannels );


        void operator=(const ProcessT& p){ *this = p;}
        
        
        void executeCommands(const QStringList commands , const QProcess::ProcessChannelMode &mode=QProcess::SeparateChannels, bool root=true, bool startNow=true);
        void execute(const QStringList command, const QProcess::ProcessChannelMode &mode );
        
        //for backwards compatibility with the old Mobloquer code
        void execute( const QString &name, const QStringList &args, const QProcess::ProcessChannelMode &mode = QProcess::SeparateChannels );
        bool allFinished() { return m_Commands.isEmpty() && (! this->isRunning()); }
            
	signals:
		/**
		 * Emitted when a command has finished running.
		 * @param output The output of the command which was executed.
		 */
		void commandOutput( QString output );
        void allCmdsFinished(QStringList commands);
		
	private:
		QString m_Command;
        QStringList m_Commands;
        QStringList m_ExecutedCommands;
		QStringList m_Args;
		QProcess::ProcessChannelMode m_ChanMode;
		QString m_Output;
        QTimer m_Timer;
        bool m_NeedsRoot;
        
    private slots:
        void executeCommand(const QString command="", const QProcess::ProcessChannelMode &mode = QProcess::SeparateChannels, bool root = false, bool startNow = true);

};

#endif //PROC_THREAD_H
