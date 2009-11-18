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

#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QObject>

#include <QString>

#include "abstracthandler.h"

#define MAX_LOG_SIZE 100
#define 

enum LogItemType {
    PGLD_IN, //pgld blocked an incoming connection
    PGLD_OUT, //pgld blocked an outgoing connection
    PGLD_FWD, //pgld blocked a forwarded connection
    PGLD_SYSTEM, //pgld system message
    PGLD_ERROR, //pgld error message
    PGLC_SYSTEM, //pglcmd system message
    PGLC_ERROR, //pglcmd error message
    PGLG_INGORE //an invalid entry which should be ignored
};

struct LogItem {
    QString m_RawMessage; //The raw data string from the log
    QString m_Date; //The date of the specific log entry
    QString m_Time; //The time of the specific log entry
    LogItemType m_Type; //The type of this entry
    //This either contains the description of the blocked host(in case of a hit entry) or the message without its timestamp
    QString m_Details;
    //The strings below are useful only for PGLD_IN, PGLD_OUT and PGLD_ERROR items.
    QString m_DestIP; //The IP of the destination of the connection
    QString m_DestPort; //The port of the destination of the connection
    QString m_SrcIP; //The IP of the source of the connection
    QString m_SrcPort; //The port of the source of the connection
    QString m_Protocol; //The protocol of the connection
};

class LogHandler : public AbstractHandler, public QObject {

    Q_OBJECT

    public:
        LogHandler();
        ~LogHandler();
        LogItem getItemByIP( const QString &ip, const QString &type ) const;
        LogItem getItemByName( const QString &name ) const;

    public slots:
        requestNewItem();
        clear();

    signals:
        newLogItem( const LogItem & );
        newLogItemHits( const LogItem & );

    private:
        QVector< LogItem > m_LogItemV;

};

#endif //LOGHANDLER_H