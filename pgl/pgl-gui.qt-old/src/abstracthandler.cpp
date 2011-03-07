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

#include <QFile>
#include <QString>

#include "debug.h"

#include "abstracthandler.h"


AbstractHandler::AbstractHandler() {


}


AbstractHandler::~AbstractHandler() {


}


QString AbstractHandler::getFilePath( const QString &id  ) const {

    QHash< QString, QString >::const_iterator s = m_FilePaths.find( id );
    if ( s != m_FilePaths.end() ) {
        return *s;
    }
    else {
        WARN_MSG << "File path" << id << "is not set!";
        return QString();
    }

}


void AbstractHandler::setFilePath( const QString &path, const QString &id ) {


    if ( path.isEmpty() ) {
        WARN_MSG << "Could not initialize object: Empty file path given!";
    }
    else if ( !QFile::exists( path ) ) {
        WARN_MSG << Q_FUNC_INFO << "Could not initialize object: File" << path << "does not exist!";
    }
    else {
        m_FilePaths.insert( id, path );
    }

}

#include "abstracthandler.moc" //Required for cmake, do not remove
