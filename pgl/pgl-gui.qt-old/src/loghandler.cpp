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

#include <QRegExp>
#include <QStringList>
#include <QDateTime>

#include "debug.h"

#include "loghandler.h"


LogHandler::LogHandler( const QString &regexPath, QObject *parent ) :
    QObject( parent ),
    m_RegExFile( regexPath ) {

}

LogHandler::LogHandler( const QString &daemonPath, const QString &cmdPath, const QString &regexPath, QObject *parent ) :
    QObject( parent ),
    m_RegExFile( regexPath ) {

    setFilePaths( daemonPath, cmdPath );

}

LogHandler::~LogHandler() {

}

void LogHandler::setFilePaths( const QString &daemonPath, const QString &cmdPath ) {

    AbstractHandler::setFilePath( daemonPath, DAEMON_FILE_ID );
    AbstractHandler::setFilePath( cmdPath, CMD_FILE_ID );

}

bool LogHandler::isWorking() const {

    if ( AbstractHandler::getFilePath( DAEMON_FILE_ID ).isEmpty() ) {
        return false;
    }
    if ( AbstractHandler::getFilePath( CMD_FILE_ID ).isEmpty() ) {
        return false;
    }

    return true;

}

void LogHandler::clear() {

    m_LogItemL.clear();

}

void LogHandler::requestDaemonItem() {

}

void LogHandler::requestCmdItem() {


}

LogItem LogHandler::parseDaemonEntry( const QString &entry ) const {

    LogItem newItem;

    newItem.m_RawMessage = entry;
    newItem.m_Type = PGLG_INGORE;

    //Separate the time and date from the rest of the item
    if ( !m_RegExFile.hasData() ) {
        WARN_MSG << "Regular expressions file seems to be empty. Cannot parse log!";
        return newItem;
    }

    //Intiallize the regural expressions from the regexps text file
    QRegExp td( m_RegExFile.getLine( REGEX_FILE_DAEMON_TIMEDATE ) );
    QRegExp hitMsg( m_RegExFile.getLine( REGEX_FILE_DAEMON_HIT ) );
    QRegExp systemMsg( m_RegExFile.getLine( REGEX_FILE_DAEMON_SYSTEM ) );

    //Separate time and date
    td.indexIn( entry );
    newItem.m_Date = td.cap(1);
    newItem.m_Time = td.cap(2);
    //Or maybe(in order to have one common format in all the logs):
    //newItem.m_Date = QDate( QDate::currentDate() ).toString("dd MMM yyyy");
    //newItem.m_Time = QTime( QTime::currentTime() ).toString();
    QString msg = td.cap(3);

    if ( msg.isEmpty() ) {
        WARN_MSG << "Failed to parse log entry:" << entry;
        return newItem;
    }

    //Check if it's a general system message
    systemMsg.indexIn( msg );
    if ( !systemMsg.cap(1).isEmpty() ) {
        newItem.m_Type = PGLD_SYSTEM;
        newItem.m_Details = systemMsg.cap(1);
    }
    else {
        //If we got here, it's a hit entry
        hitMsg.indexIn( msg );
        qDebug() << hitMsg.capturedTexts();
        //Check if the RegExp worked(maybe not the best way to do so) and use it
        if ( ! hitMsg.cap(1).isEmpty() ) {
            //The following depend on the format of the specific entry
            //Written to parse strings with the following format: "Date CHAIN: SRC_IP:PORT DEST_IP:PORT PROTO || Label"
            QString itype = hitMsg.cap(1);
            newItem.m_SrcIP = hitMsg.cap(2);
            newItem.m_SrcPort = hitMsg.cap(3);
            newItem.m_DestIP = hitMsg.cap(4);
            newItem.m_DestPort = hitMsg.cap(5);
            newItem.m_Protocol = hitMsg.cap(6);
            newItem.m_Details = hitMsg.cap(7);

            //If the item was parsed sucessfully, set the correct item type
            if ( itype == "IN" ) {
                newItem.m_Type = PGLD_IN;
            }
            else if ( itype == "OUT" ) {
                newItem.m_Type = PGLD_OUT;
            }
            else if ( itype == "FWD" ) {
                newItem.m_Type = PGLD_FWD;
            }
        }
        else {
            WARN_MSG << "Failed to parse hit(?) entry:" << entry;
        }
    }



    return newItem;


}

LogItem LogHandler::parseCmdEntry( const QString &entry ) const {

    //pglcmd entries can have one of the following formats:
    //2009-12-07 17:47:37 CET Begin: pglcmd start
    //...Information...
    //2009-12-07 17:47:44 CET End: pglcmd start
    //Error [errorcode]: description

    QRegExp detsep( m_RegExFile.getLine( REGEX_FILE_CMD_BASIC ) );
    detsep.indexIn(entry);

    LogItem newItem;

    newItem.m_RawMessage = entry;
    //No need to waste time parsing the time/date
    newItem.m_Date = QDate( QDate::currentDate() ).toString("dd MMM yyyy");
    newItem.m_Time = QTime( QTime::currentTime() ).toString();

    if ( entry.contains( "error", Qt::CaseInsensitive ) ) { //Format: "Error [errorcode]: description"
        newItem.m_Type = PGLC_ERROR;
    }
    else if ( entry.contains( "Begin:", Qt::CaseInsensitive ) ) { //Format: "2009-12-07 17:47:37 CET Begin: pglcmd start"
        newItem.m_Type = PGLC_BEGIN;
    }
    else if ( entry.contains( "End:", Qt::CaseInsensitive ) ) { //Format: "2009-12-07 17:47:44 CET End: pglcmd start"
        newItem.m_Type = PGLC_END;
    }
    else {
        newItem.m_Type = PGLC_SYSTEM; //Any other entry goes here
    }

    newItem.m_Details = detsep.cap(1);

    return newItem;


}

#include "loghandler.moc"
