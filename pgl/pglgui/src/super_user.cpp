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

#include <QDebug>

#include "utils.h"
#include "file_transactions.h"
#include "pgl_settings.h"

QString mSudoCmd = "";
bool mGetSudoCommand = false;

SuperUser::SuperUser( QObject *parent, const QString& rootpath ):
    QObject(parent)
{
    m_ProcT = new ProcessT(this);
    connect(m_ProcT, SIGNAL(finished(const CommandList&)), this, SLOT(processFinished(const CommandList&)));
    mGetSudoCommand = false;
    
    if (mSudoCmd.isEmpty() && ! mGetSudoCommand) {
        mSudoCmd = rootpath;
        if ( rootpath.isEmpty() )
            findGraphicalSudo();
    }
}


SuperUser::~SuperUser() 
{
    QFile tmpfile(TMP_SCRIPT);
    
    if ( tmpfile.exists() )
        tmpfile.remove();
    
    m_ProcT->wait();
}

void SuperUser::findGraphicalSudo()
{
    QStringList gSudos;
    QStringList prefixes;

    gSudos << KDESUDO << KDESU << GKSUDO << GKSU;
    prefixes << PREFIX1 << PREFIX2;
    
    QDir prefix3 = QDir::home();
    if (prefix3.cd(".local") && prefix3.cd("bin"))
        prefixes << prefix3.absolutePath();
    
    for(int i=0; i < prefixes.size(); i++) 
        for(int j=0; j < gSudos.size(); j++) 
            if (QFile::exists(prefixes[i] + gSudos[j])) {
                mSudoCmd = prefixes[i] + gSudos[j];
                return;
            }
            
    
    //if the graphical sudo hasn't been found yet, try the 'which' command as last resource.
    for(int i=0; i < gSudos.size(); i++) 
        gSudos[i].insert(0, "which ");

    mGetSudoCommand = true;
    m_ProcT->executeCommands(gSudos);
}

QString SuperUser::getRootPath()
{
    return mSudoCmd;
}
	
void SuperUser::executeCommand(QString& command, bool start) 
{
    executeCommands(QStringList() << command, start);
}

void SuperUser::executeCommands(QStringList commands, bool start) 
{
    if ( (! start) || commands.isEmpty())
    {
        m_Commands << commands;
        return;
    }

    if ( mSudoCmd.isEmpty() || (! QFile::exists(mSudoCmd)) )
    {
        QString errorMsg = tr("Could not use either kdesu(do) or gksu(do) to execute the command requested.\
        You can set the path of the one you prefer in <b>\"Options - Settings - Sudo front-end\"</b>");
        qCritical() << errorMsg;
        emit error(errorMsg);
        return;
    }
    
    if ( m_ProcT->isRunning() )
    {
        qWarning() << "Another process is still running...";
        emit error("Another process is still running...");
        return;
    }

    if ( ! hasPermissions("/etc") )//If the program is not run by root, use kdesu or gksu as first argument
    {
        for (int i=0; i < commands.size(); i++)
        {
            commands[i].insert(0, mSudoCmd + " ");
        }
    }
    
    qDebug() << "Executing commands: \n" << commands << "\n";

    m_ProcT->executeCommands(commands);
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

    //QStringList lines;
    //QString cmd = QString("sh %1").arg(TMP_SCRIPT);
    QString cmd = QString("sh -c \"(%1)\"").arg(m_Commands.join(") && ("));
    executeCommand(cmd);

    //create file with the commands to be executed
    /*lines << "#!/bin/sh";
    lines << "set -e";
    lines << m_Commands;
    
    bool ok = saveFileData(lines, "/tmp/execute-all-pgl-commands.sh");

    if ( ok )
        executeCommand(cmd);*/
    
}

void SuperUser::processFinished(const CommandList& commands)
{
    if ( ! m_Commands.isEmpty() )
        m_Commands.clear();
    
    CommandList failedCommands;
    foreach(const Command& cmd, commands) {
        if (cmd.error())
            failedCommands << cmd;
    }
    
    if (mGetSudoCommand && ! commands.isEmpty()) {
        Command command = commands.first();
        if ( command.contains("which") && mSudoCmd.isEmpty() && QFile::exists(command.output())) {
            mSudoCmd = command.output();
            mGetSudoCommand = false;
        }
    }
    
    
    if (! failedCommands.isEmpty()) {
        emit error(failedCommands);
    }

    emit finished();
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
        foreach(const QString& key, files.keys()) {
            if (key.isEmpty() || files[key].isEmpty()) 
                continue;
            commands << QString("mv %1 %2").arg(key).arg(files[key]);
        }
        
        executeCommands(commands, start);
    }
}

/*** Static methods ***/

QString SuperUser::sudoCommand()
{
    return mSudoCmd;
}

void SuperUser::setSudoCommand(const QString& cmd)
{
    mSudoCmd = cmd;
}

