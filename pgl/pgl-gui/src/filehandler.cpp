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
#include <QString>
#include <QVector>
#include <QFile>
#include <QTextStream>

#include <QDebug>

#include "filehandler.h"

FileHandler::FileHandler( RawData *parent ) :
    RawData( parent ) {

    
}

FileHandler::FileHandler( const QString &filename, RawData *parent ) :
    RawData( parent ) {

    this->open( filename );

}

FileHandler::~FileHandler() {


}

QString FileHandler::getFilename() const {

    return m_Filename;

}

void FileHandler::setData( const QVector< QString > &newD ) {


    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to set data of FileHandler to NULL!";
    }

    RawData::m_RawDataVector = newD;

    this->trimLines();

}

void FileHandler::setData( const QString &newD ) {

    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to set data of FileHandler to NULL!";
    }
    //Clear the vector and make this string its only element
    RawData::m_RawDataVector.clear();
    RawData::m_RawDataVector.append( newD.trimmed() );

}

void FileHandler::appendData( const QVector< QString > &newD ) {


    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to append an empty data vector!";
    }

    RawData::m_RawDataVector += newD ;

    this->trimLines();
    
}

void FileHandler::appendData( const QString &newD ) {


    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to append an empty data string!";
    }

    RawData::m_RawDataVector.append( newD.trimmed() );

}

bool FileHandler::operator==( const FileHandler &second ) const {

    //If one file is empty
    if ( ! ( this->hasData() && second.hasData() ) ) {
        return false;
    }

    return ( this->RawData::m_RawDataVector == second.RawData::m_RawDataVector );

}

void FileHandler::operator=( const FileHandler &second ) {


    this->RawData::m_RawDataVector = second.RawData::m_RawDataVector;

}


bool FileHandler::open( const QString &filename ) {

    QFile file( filename );
    if ( filename.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Empty file filename given, doing nothing.";
        return false;
    }
    else if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        qWarning() << Q_FUNC_INFO << "Could not read from file:" << filename;
        return false;
    }
    m_Filename = filename;
    QTextStream in( &file );
    while ( !in.atEnd() ) {
        QString line = in.readLine();
        line = line.trimmed();
        RawData::m_RawDataVector.append(line);
    }

    return true;

}

bool FileHandler::close() {


    RawData::m_RawDataVector.clear();
    m_Filename.clear();

    return true;

}

bool FileHandler::save( const QString &filename ) {

    if ( ! filename.isNull() ) {
        m_Filename = filename;
    }
        
    QFile file( m_Filename );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        qWarning() << Q_FUNC_INFO << "Could not write to file:" << m_Filename;
        return false;
    }
    QTextStream out(&file);
    for ( QVector< QString >::const_iterator s = RawData::m_RawDataVector.begin(); s != RawData::m_RawDataVector.end(); s++ ) {
        out << *s << "\n";
    }

    return true;

}

void FileHandler::trimLines() {


    for ( QVector< QString >::iterator s = RawData::m_RawDataVector.begin(); s != RawData::m_RawDataVector.end(); s++ ) {
        *s = s->trimmed();
    }

}


void FileHandler::requestNewData() {



    //Reload the file
    RawData::m_RawDataVector.clear();
    if ( this->open( m_Filename ) == false ) {
        qWarning() << Q_FUNC_INFO << "Could not reload file:" << m_Filename;
        return;
    }
    
    emit RawData::rawDataV( RawData::getDataV() );
    //emit RawData::RawDataS( RawData::GetDataS() );

}

#include "filehandler.moc" //Required for CMake, do not remove.
