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


#include <QApplication>
#include <QDir>
#include <QStringList>
#include <QFile>

#include <cstdio>
#include <cstdlib>

#include "peerguardian.h"

void customOutput( QtMsgType type, const char *msg );

int main(int argc, char *argv[])
{

	qInstallMsgHandler( customOutput );

	//Start the real application
	QApplication app(argc, argv);
	//Set the application information here so QSettings objects can be easily used later.
	QApplication::setOrganizationName( "pgl" );
	QApplication::setOrganizationDomain( "https://sourceforge.net/projects/peerguardian" );
	QApplication::setApplicationName( "pgl-gui" );
    app.setQuitOnLastWindowClosed(false);

	Peerguardian pgWindow;
    
    pgWindow.addApp(app);

	QStringList args = QApplication::arguments();
	//If tray argument was not given show the window normally
	//Otherwise show minimized in tray
	pgWindow.setVisible( !args.contains( "--tray" ) );

	return app.exec();

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
