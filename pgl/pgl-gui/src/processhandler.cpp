/**************************************************************************
*   Copyright (C) 2009-2010 by Dimitris Palyvos-Giannas                   *
*   jimaras@gmail.com                                                     *
*                                                                         *
*   This is a part of pgl-gui.                                            *
*   pgl-gui is free software; you can redistribute it and/or modify       *
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

#include <QStringList>

#include "debug.h"

#include "processhandler.h"

ProcessHandler::ProcessHandler( QObject *parent ) :
    QThread( parent ) {

    DEBUG_MSG << "New thread created.";
    
}

ProcessHandler::~ProcessHandler() {

    wait(); //Wait for the thread to finish its job
    DEBUG_MSG << "Thread destroyed";

}

void ProcessHandler::setCommand( const QString &cmd, const QProcess::ProcessChannelMode &mode  ) {

    if ( cmd.isEmpty() ) {
        WARN_MSG << "No process name set, doing nothing.";
        return;
    }

    //Separate the command and its arguments
    m_Args = cmd.split( " ", QString::SkipEmptyParts );
    m_Cmd = m_Args.first();
    m_Args.removeFirst();

}

void ProcessHandler::runCommand( const QString &cmd, const QProcess::ProcessChannelMode &mode ) {

    setCommand( cmd, mode );

    if ( !this->isRunning() ) {
        start();
    }
    else {
        WARN_MSG << "Thread already running, doing nothing.";
        return;
    }

}

void ProcessHandler::run() {

    if ( m_Cmd.isEmpty() ) {
        WARN_MSG << "No process name set, doing nothing.";
        return;
    }
    

    QString poutput;

    DEBUG_MSG << "Executing command:" << m_Cmd << m_Args;
    QProcess proc;
    proc.setProcessChannelMode( m_ChanMode );
    proc.start( m_Cmd, m_Args );
    proc.waitForStarted();
    proc.waitForFinished();
    proc.closeWriteChannel();
    
    poutput = proc.readAll().trimmed();

    emit processDataS( poutput );
    DEBUG_MSG << "Command execution finished.";

}

#include "processhandler.moc" //Required for CMake, do not remove.
