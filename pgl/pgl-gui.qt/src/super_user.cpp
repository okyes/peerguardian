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

#include "super_user.h"
#include "utils.h"


QString SuperUser::m_TestFile = TEST_FILE_PATH;

QString SuperUser::m_SudoCmd = "";

SuperUser::SuperUser( QObject *parent ) :
	ProcessT( parent )
{
}

SuperUser::SuperUser( QString& rootpath,  QObject *parent ) :
	ProcessT( parent )
{
    setRootPath(rootpath, true);
}

SuperUser::~SuperUser() {

	QSettings tempSettings;
	tempSettings.setValue( "paths/super_user", m_SudoCmd );

}

void SuperUser::setRootPath(QString& path, bool verified)
{
    if ( verified )
        m_SudoCmd = path;
    else
        m_SudoCmd = SuperUser::getFilePath(path);
}

QString SuperUser::getRootPath()
{
    return m_SudoCmd;
}

void SuperUser::setFilePath( const QString &path ) {


	if ( !path.isEmpty() ) {
		if ( QFile::exists( path ) ) {
			m_SudoCmd = path;
		}
		else {
			qCritical() << Q_FUNC_INFO << "Could not set sudo front-end path to:" << path << "as the file does not exist.";
		}
	}
	else {
		qCritical() << Q_FUNC_INFO <<  "Could not change sudo front-end's path. Empty path given.";
	}

		

}
	

void SuperUser::setTestFile( const QString &path ) {

	if ( QFile::exists( path ) ) {
		m_TestFile = path;
	}
	else {
		qDebug() << Q_FUNC_INFO << "Could not set SuperUser test file to:" << path << "as this file does not exist.";
	}

}
	

void SuperUser::execute( const QStringList &command ) {

	if ( m_SudoCmd.isEmpty() ) {
		qCritical() << Q_FUNC_INFO << "Could not use either kdesu or gksu to execute the command requested.\nIf neither of those executables exist in /usr/bin/ you can set the path of the one you prefer in mobloquer's configuration file, usually found in ~/.config/mobloquer/mobloquer.conf.\nThe path can be changed by changing the value of the super_user key, or creating a new one if it doesn't already exist.";
		return;
	}


	//Check if mobloquer was started with root privilleges
	QFileInfo test( m_TestFile );
	if ( !test.exists() ) {
		qDebug() << Q_FUNC_INFO << "Could not use test file" << m_TestFile << "as it does not exist.";
	}
	if ( test.isReadable() && test.isWritable() ) { //If the program is run by root
		QString cmd = command.first();
		QStringList newCommand = command;
		newCommand.removeFirst();
		qDebug() << cmd << newCommand;
		ProcessT::execute( cmd, newCommand, QProcess::MergedChannels );
		return; 
	}

	ProcessT::execute( m_SudoCmd, command, QProcess::MergedChannels );

	return; 

}

void SuperUser::moveFile( const QString &source, const QString &dest ) {

	execute( QStringList() << "mv" << source << dest );

}

void SuperUser::copyFile( const QString &source, const QString &dest ) {

	execute( QStringList() << "cp" << source << dest );

}

void SuperUser::removeFile( const QString &source ) {

	moveFile( source, "/dev/null" );

}

/*** Static methods ***/

QString SuperUser::getFilePath()
{
    QString path("");
    return getFilePath(path);
}

QString SuperUser::getFilePath(const QString &path)
{
    QString p = getValidPath(path, KDESU_PATH);
    if ( p.isEmpty() )
        p = getValidPath(path, GKSU_PATH);
        
    return p;
}

