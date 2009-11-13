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

#include <QObject>



#include <QDebug>

#include "rawdata.h"


RawData::RawData( QObject *parent ) :
    QObject( parent ) {

}

RawData::~RawData() {

}

bool RawData::hasData() const {

    return ( this->getSize() > 0 );

}

int RawData::getSize() const {

    return m_RawDataVector.size();

}

QString RawData::getDataS() const {

    QString data;
    for ( QVector< QString >::const_iterator s = m_RawDataVector.begin(); s != m_RawDataVector.end(); s++ ) {
        data.append( *s );
        data.append('\n');
    }

    return data;

}

QVector< QString > RawData::getDataV() const {

    return m_RawDataVector;

}

void RawData::requestData() {
    
    
    emit rawDataV( m_RawDataVector );
    
}


#include "rawdata.moc" //Required for CMake, do not remove.

