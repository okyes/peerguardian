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

#include "proc_thread.h"


ProcessT::ProcessT( QObject *parent ) :
	QThread( parent )
{
	m_ChanMode = QProcess::SeparateChannels;
}


ProcessT::~ProcessT() {

	wait();
	qDebug() << Q_FUNC_INFO << "Thread destroyed";

}

void ProcessT::run() {


	if ( m_Command.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "No command set, doing nothing";
		return;
	}

	qDebug() << Q_FUNC_INFO << "Executing command" << m_Command << m_Args << "...";
	QProcess proc;
	proc.setProcessChannelMode( m_ChanMode );
	//proc.start( m_Command, m_Args );
    proc.start( m_Command ); 
	proc.waitForStarted();
	proc.waitForFinished();
	proc.closeWriteChannel();
	
	m_Output = proc.readAll().trimmed();

    qDebug() << m_Output;
        
	emit commandOutput( m_Output );
	qDebug() << Q_FUNC_INFO << "Command execution finished.";
    
    if ( m_Commands.isEmpty() )
        emit allCmdsFinished(m_ExecutedCommands);
    else
    {
        m_ExecutedCommands << m_Commands.first();
        executeCommand(m_Commands.takeFirst(), m_ChanMode);
    }

}

void ProcessT::setCommand( const QString &name, const QStringList &args, const QProcess::ProcessChannelMode &mode ) {

    //***** This function is for backwards compatibility with the old Mobloquer code. *****//
    //***** It should be removed at some point.*****//

	if ( name.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Empty command given, doing nothing";
		return;
	}

	m_Command = name;
	m_Args = args;
	m_ChanMode = mode;

}

void ProcessT::executeCommand(const QString command, const QProcess::ProcessChannelMode &mode, bool startNow) {

    m_ChanMode = mode;
    m_Command = command;

	if ( ! isRunning() && startNow )
		start();

}


void ProcessT::execute( const QString &name, const QStringList &args, const QProcess::ProcessChannelMode &mode ) {

    //***** This function is for backwards compatibility with the old Mobloquer code. *****//
    //***** It should be removed at some point.*****//

	//setCommand( name, args, mode );
    QString cmd = QString("%1 %2").arg(name).arg(args.join(" "));
    
    executeCommand(cmd, mode);

}


void ProcessT::execute(const QStringList command, const QProcess::ProcessChannelMode &mode ) 
{
    //***** This function is for backwards compatibility with the old Mobloquer code. *****//
    //***** It should be removed at some point.*****//
    
    //QString name = command.first();
    //QStringList args = command.mid(1);

	//setCommand( name, args, mode );
    m_Commands.clear();
    executeCommand(command.join(" "), mode);

}



void ProcessT::executeCommands(const QStringList commands , const QProcess::ProcessChannelMode &mode, bool startNow) {

    if ( commands.isEmpty() )
        return;

    m_Commands.clear();
    m_Commands << commands;
    
    executeCommand(m_Commands.takeFirst(), mode, startNow);

}


