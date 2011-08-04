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


QString SuperUser::m_SudoCmd = "";



SuperUser::SuperUser( QObject *parent ):
    QObject(parent)
{
    m_parent = parent;
}

SuperUser::SuperUser( QString& rootpath,  QObject *parent ):
    QObject(parent)
{
    setRootPath(rootpath, true);
    m_parent = parent;
}

SuperUser::~SuperUser() {

	QSettings tempSettings;
	tempSettings.setValue( "paths/super_user", m_SudoCmd );

    foreach(ProcessT *t, m_threads)
    {
        if ( t->isRunning() )
        {
            t->quit();
            t->wait();
        }
        t->deleteLater();
    }
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


	if ( !path.isEmpty() )
    {
		if ( QFile::exists( path ) )
			m_SudoCmd = path;
		else
			qCritical() << Q_FUNC_INFO << "Could not set sudo front-end path to:" << path << "as the file does not exist.";
	}
	else
		qCritical() << Q_FUNC_INFO <<  "Could not change sudo front-end's path. Empty path given.";
	
}
	


void SuperUser::executeCommands(QStringList commands, bool wait) 
{
    
    
    QProcess::ProcessChannelMode mode = QProcess::MergedChannels;
    ProcessT *t;

    if ( wait )
        m_Commands << commands;

	if ( m_SudoCmd.isEmpty() )
    {
		qCritical() << Q_FUNC_INFO << "Could not use either kdesu or gksu to execute the command requested.\nIf neither of those executables exist in /usr/bin/ you can set the path of the one you prefer in pgl-gui's configuration file, usually found in ~/.config/pgl/pgl-gui.conf.\nThe path can be changed by changing the value of the super_user key, or creating a new one if it doesn't already exist.";
		return;
	}


	if ( ! hasPermissions("/etc") )//If the program is not run by root, use kdesu or gksu as first argument
    {
        for (int i=0; i < commands.size(); i++)
        {
            commands[i].insert(0, m_SudoCmd + " \"");
            commands[i].append("\"");
        }
	}
    
    qDebug() << commands;
    qDebug() << "start thread";

    t = new ProcessT(m_parent);
    m_threads.push_back(t);
    
    if ( m_threads.size() == 1 )
        t->executeCommands(commands, mode);
    else
        t->executeCommands(commands, mode, false);
    
        
    connect(t, SIGNAL(allCmdsFinished(QStringList)), this, SLOT(processFinished(QStringList)));

}

void SuperUser::execute(const QStringList& command ) 
{
    QStringList commands;
    commands << command.join(" ");
    executeCommands(commands);
}

/*void SuperUser::startThread(const QString &name, const QStringList &args, const QProcess::ProcessChannelMode &mode )
{
    //Although it could use several ProcessT instances at the same time,
    //the way it works now will only allow to have one ProcessT instance at a time,
    //because if we had several, each one of them would pop-up a dialog asking for
    //the root password and that would be very annoying to the user.

    bool executing = false;
    qDebug() << "start thread";
    foreach(ProcessT *t, m_threads)
    {
        if ( t->isFinished() )
        {
            t->execute(name, args, mode);
            executing = true;
            break;
        }
    }

    if ( ! executing )
    {
        ProcessT *t = new ProcessT(m_parent);
        m_threads.push_back(t);
        t->execute(name, args, mode);
        connect(t, SIGNAL(allCmdsFinished()), this, SLOT(processFinished()));
    }


}*/

void SuperUser::executeAll()
{
    executeCommands(m_Commands, false);
}

void SuperUser::processFinished(QStringList commands)
{
    ProcessT * t;
    
    m_Commands.clear();
    
    emit(finished());
    
    if ( ! m_threads.isEmpty() )
    {
        t = m_threads.takeFirst();
        delete t;
        
        if ( ! m_threads.isEmpty() )
            m_threads.first()->start();
    }
}


void SuperUser::moveFile( const QString &source, const QString &dest ) 
{
	execute( QStringList() <<  "mv" << source << dest );
}

void SuperUser::copyFile( const QString &source, const QString &dest ) 
{
	execute( QStringList() <<  "cp" << source << dest );
}

void SuperUser::removeFile( const QString &source ) {
	moveFile( source, "/dev/null" );
}


void SuperUser::moveFiles( const QMap<QString, QString> files, bool wait)
{
    if ( ! files.isEmpty() )
    {
        QStringList commands;
        QString cmd;
        foreach(QString key, files.keys())
        {
            cmd = QString("mv %1 %2").arg(key).arg(files[key]);
            commands << cmd;
        }
        
        executeCommands(commands, wait);
    }
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




