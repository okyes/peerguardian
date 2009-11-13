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

#include <QDebug>
#include <QStringList>

#include "processhandler.h"

ProcessHandler::ProcessHandler( QThread *parent1, RawData *parent2 ) :
    QThread(parent1), RawData( parent2 ) {

    qDebug() << Q_FUNC_INFO << "New thread created.";

    this->setChannelMode( QProcess::SeparateChannels );
    
}

ProcessHandler::ProcessHandler( const QProcess::ProcessChannelMode &mode, QThread *parent1, RawData *parent2 ) :
    QThread(parent1), RawData( parent2 ) {

    this->setChannelMode( mode );

}

ProcessHandler::~ProcessHandler() {

    wait(); //Wait for the thread to finish its job
    qDebug() << Q_FUNC_INFO << "Thread destroyed";

}

QString ProcessHandler::getProcessName() const {

    QString name = m_Cmd;
    name.append( m_Args.join( " " ) );

    return name;

}

void ProcessHandler::setChannelMode( const QProcess::ProcessChannelMode &mode ) {

    m_ChanMode = mode;

}

bool ProcessHandler::open( const QString &process ) {

    if ( process.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "No process name set, doing nothing.";
        return false;
    }

    //Separate the command and its arguments
    m_Args = process.split( " ", QString::SkipEmptyParts );
    m_Cmd = m_Args.first();
    m_Args.removeFirst();
    

    return true;

}

void ProcessHandler::run() {

    if ( m_Cmd.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "No process name set, doing nothing.";
        return;
    }
    

    QString poutput;

    qDebug() << Q_FUNC_INFO << "Executing command:" << getProcessName();
    QProcess proc;
    proc.setProcessChannelMode( m_ChanMode );
    proc.start( m_Cmd, m_Args );
    proc.waitForStarted();
    proc.waitForFinished();
    proc.closeWriteChannel();
    
    poutput = proc.readAll().trimmed();
    
    RawData::m_RawDataVector = poutput.split( "\n" ).toVector();
    
    emit RawData::rawDataS( poutput ); //Saves some time. No need to call GetDataS
    emit RawData::rawDataV( RawData::getDataV() );
    qDebug() << Q_FUNC_INFO << "Command execution finished.";

}
    

bool ProcessHandler::close() {

    qWarning() << Q_FUNC_INFO << "Terminating process:" << getProcessName();
    wait(); //Maybe remove?
    QThread::terminate();

}

void ProcessHandler::requestNewData() {

    if ( !isRunning() ) {
        start();
    }
    else {
        qWarning() << Q_FUNC_INFO << "Thread already running, doing nothing.";
        return;
    }

}
    

#include "processhandler.moc" //Required for CMake, do not remove.