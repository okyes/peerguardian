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
#include <QVector>
#include <QString>

#include "filehandler.h"
#include "processhandler.h"

void customOutput( QtMsgType type, const char *msg );

int main() {

    qInstallMsgHandler( customOutput );
    ProcessHandler test;
    test.Open( "sleep 2m" );
    test.Close();
    return 0;

}

void customOutput( QtMsgType type, const char *msg ) {
    
    switch( type ) {
        case QtDebugMsg:
            fprintf( stderr, "** Debug: %s\n", msg );
            break;
        case QtWarningMsg:
            fprintf( stderr, "** Warning: %s\n", msg );
            break;
        case QtCriticalMsg:
            fprintf( stderr, "** Critical: %s\n", msg );
            break;
        case QtFatalMsg:
            fprintf( stderr, "** Fatal: %s\n", msg );
            break;
    }
    
}