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

#include <QVector>
#include <QString>

#include <QDebug>

#include "rawdata.h"


RawData::RawData( const QObject *parent ) : QObject( parent ) {

}

RawData::~RawData() {

}

QString &RawData::GetDataS() const {

    qWarning() << Q_FUNC_INFO << "This function shouldn't have been called!\nReturning empty QString.";
    
    return QString();

}

QVector< QString > &RawData::GetDataV() const {


    qWarning() << Q_FUNC_INFO << "This function shouldn't have been called!\nReturning empty QVector< QString >.";
    
    return QVector < QString > ();

}

bool RawData::Save() {

    qWarning() << Q_FUNC_INFO << "This function shouldn't have been called!\nUse it only with objects which handle files.\nDoing nothing.";
    
}