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

#include "file_transactions.h"



QVector< QString > getFileData( const QString &path ) {

	QVector< QString > fileContents;
	QFile file( path );
	if ( path.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Empty file path given, doing nothing";
		return fileContents;
	}
	else if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		qWarning() << Q_FUNC_INFO << "Could not read from file" << path;
		return fileContents;
	}
	QTextStream in( &file );
	while ( !in.atEnd() ) {
		QString line = in.readLine();
		line = line.trimmed();
		fileContents.push_back(line);
	}

	return fileContents;

}

bool saveFileData( const QVector< QString > &data, const QString &path ) {

	QFile file( path );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
		qWarning() << Q_FUNC_INFO << "Could not write to file" << path;
		return false;
	}
	QTextStream out(&file);
	for ( QVector< QString >::const_iterator s = data.begin(); s != data.end(); s++ ) {
		out << *s << "\n";
	}
	return true;
}

bool compareFileData( const QString &pathA, const QString &pathB ) {

	QVector< QString > fileA = getFileData( pathA );
	QVector< QString> fileB = getFileData( pathB );

	if ( fileA.isEmpty() || fileB.isEmpty() ) {
		return false;	
	}
	
	if ( fileA == fileB ) {
		return true;
	}

	return false;

}
