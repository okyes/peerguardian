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


#include "pglcmd.h"
#include "super_user.h"
#include "utils.h"

PglCmd::PglCmd(  const QString &path, QObject *parent ) :
	SuperUser( parent )
{	
	setFilePath(path);
}

PglCmd::PglCmd( QObject *parent ) :
	SuperUser( parent )
{
}

QString PglCmd::getPath()
{
    return m_FileName;
}

void PglCmd::setFilePath( const QString &path, bool verified) {

    if ( verified )
        m_FileName = path;
    else
        m_FileName = getFilePath(path);

    if ( m_FileName.isEmpty() ){
        qCritical() << Q_FUNC_INFO << "File " << m_FileName << " could not be found.";
		qCritical() << Q_FUNC_INFO << "PglCmd will probably not work";
    }
}



void PglCmd::start() {

	SuperUser::execute( QStringList() << m_FileName << "start" );
	emit actionMessage( tr( "Starting Peerguardian..." ), MESSAGE_TIMEOUT );

}

void PglCmd::restart() {

	SuperUser::execute( QStringList() << m_FileName << "restart" );
	emit actionMessage( tr( "Restarting Peerguardian..." ), MESSAGE_TIMEOUT );

}

void PglCmd::stop() {

	SuperUser::execute( QStringList() << m_FileName << "stop" );
	emit actionMessage( tr( "Stopping Peerguardian..." ), MESSAGE_TIMEOUT );

}

void PglCmd::reload() {

	SuperUser::execute( QStringList() << m_FileName << "reload" );
	emit actionMessage( tr( "Reloading Peerguardian..." ), MESSAGE_TIMEOUT );

}

void PglCmd::update() {

	SuperUser::execute( QStringList() << m_FileName << "update" );
	emit actionMessage( tr( "Updating Peerguardian..." ), MESSAGE_TIMEOUT );


}

void PglCmd::status() {

	SuperUser::execute( QStringList() << m_FileName << "status" );
	emit actionMessage( tr( "Getting status for Peerguardian..." ), MESSAGE_TIMEOUT );

}

/*** Static methods ***/

QString PglCmd::getFilePath()
{
    QString path("");
    return getValidPath(path, PGLCMD_PATH);
}

QString PglCmd::getFilePath(const QString &path)
{
    return getValidPath(path, PGLCMD_PATH);
}

