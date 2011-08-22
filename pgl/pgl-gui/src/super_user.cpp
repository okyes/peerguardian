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
#include "file_transactions.h"


QString SuperUser::m_SudoCmd = "";



SuperUser::SuperUser( QObject *parent, const QString& rootpath ):
    QObject(parent)
{
    m_parent = parent;
    m_ProcT = new ProcessT(this);
    connect(m_ProcT, SIGNAL(allCmdsFinished(QStringList)), this, SLOT(processFinished(QStringList)));
    connect(m_ProcT, SIGNAL(commandOutput(QString)), this, SLOT(commandOutput(QString)));
    
    qDebug() << "Graphical Sudo: " << m_SudoCmd;
    
    if ( rootpath.isEmpty() )
    {
        QStringList gSudos;
        gSudos << "which kdesudo" << "which gksudo" << "which kdesu" << "which gksu";
        m_ProcT->executeCommands(gSudos);
    }
    else
        m_SudoCmd = rootpath;
}


SuperUser::~SuperUser() {

    QFile tmpfile(TMP_SCRIPT);
    
    if ( tmpfile.exists() )
        tmpfile.remove();
    
    m_ProcT->wait();

    /*foreach(ProcessT *t, m_threads)
    {
        if ( t->isRunning() )
        {
            t->quit();
            t->wait();
        }
        t->deleteLater();
    }*/
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
	
void SuperUser::executeCommand(QString& command, bool start) 
{
    executeCommands(QStringList() << command, start);
}

void SuperUser::executeCommands(QStringList commands, bool start) 
{
    
    
    QProcess::ProcessChannelMode mode = QProcess::MergedChannels;

    if ( (! start) || commands.isEmpty())
    {
        m_Commands << commands;
        return;
    }

	if ( m_SudoCmd.isEmpty() || (! QFile::exists(m_SudoCmd)) )
    {
		QString errorMsg = "Could not use either kdesu(do) or gksu(do) to execute the command requested.\
        You can set the path of the one you prefer in <b>\"Options - Settings - Sudo front-end\"</b>";
        qCritical() << Q_FUNC_INFO << errorMsg;
        emit error(errorMsg);
        return;
	}
    
    if ( m_ProcT->isRunning() )
    {
        qWarning() << "Another process is still running...";
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
    
    qDebug() << Q_FUNC_INFO << "Executing commands: " << commands;

    m_ProcT->executeCommands(commands, mode);

    //t = new ProcessT(m_parent);
    
    //connect(t, SIGNAL(allCmdsFinished(QStringList)), this, SLOT(processFinished(QStringList)));
    //m_threads.push_back(t);
    
    //if ( m_threads.size() == 1 )
        //t->executeCommands(commands, mode, needsRoot);
    //else
     //   t->executeCommands(commands, mode, needsRoot, false);
    

}

void SuperUser::execute(const QStringList& command ) 
{
    QStringList commands;
    commands << command.join(" ");
    executeCommands(commands);
}

void SuperUser::executeScript()
{
    if ( m_Commands.isEmpty() )
        return;
    
    QStringList lines;
    QString cmd = QString("sh %1").arg(TMP_SCRIPT);
    
    qDebug() << "Commands: " << m_Commands;
    
    //create file with the commands to be executed
    lines << "#!/bin/sh";
    lines << "set -e";
    lines << m_Commands;
    
    bool ok = saveFileData(lines, "/tmp/execute-all-pgl-commands.sh");

    if ( ok )
        executeCommand(cmd);
    
}

void SuperUser::processFinished(QStringList commands)
{
    //ProcessT * t;
    
    if ( ! m_Commands.isEmpty() )
        m_Commands.clear();
    
    emit finished();
    
    /*if ( ! m_threads.isEmpty() )
    {
        t = m_threads.takeFirst();
        delete t;
        
        if ( ! m_threads.isEmpty() )
            m_threads.first()->start();
    }*/
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


void SuperUser::moveFiles( const QMap<QString, QString> files, bool start)
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
        
        executeCommands(commands, start);
    }
}

void SuperUser::commandOutput(QString path)
{
    if ( m_SudoCmd.isEmpty() &&  (! path.isEmpty()) && (! path.contains("command not found")))
    {
        m_SudoCmd = path;
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

QString SuperUser::getGraphicalSudoPath()
{
    return m_SudoCmd;
}




