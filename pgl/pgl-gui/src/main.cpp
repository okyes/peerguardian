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

#include <QObject>

#include <QVector>
#include <QString>

#include <QThread>

#include <QRegExp>

#include "debug.h"

#include "abstracthandler.h"
#include "filehandler.h"
#include "processhandler.h"




int main() {

    qInstallMsgHandler( customOutput );
    //QRegExp rx( "^(.*)\\s(\\d\\d?:\\d\\d?:\\d\\d?)\\s(.*)" );
    //rx.indexIn( "Nov 16 07:10:05 IN: 61.160.212.242:23 96.3.141.107:8085 TCP || CHINANETpossible MediaDefender" );
    
    //qDebug() << rx.capturedTexts();

    return 0;

}