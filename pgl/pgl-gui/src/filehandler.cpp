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

FileHandler::FileHandler( RawData *parent ) : RawData( parent ) {


}

FileHandler::FileHandler( const QString &filename, RawData *parent ) : RawData( parent ) {

    this->Open( filename );

}

FileHandler::~FileHandler() {

    qDebug() << Q_FUNC_INFO << "Destroying FileHandler object...";

}

bool FileHandler::HasData() const {

    return ( this->LinesNumber() > 0 );

}

int FileHandler::LinesNumber() const {

    return ( m_FileContents.size() );

}

void FileHandler::SetData( const QVector< QString > &newD ) {

    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to set data of FileHandler to NULL!";
    }

    m_FileContents = newD;

    this->TrimLines();

}

void FileHandler::SetData( const QString &newD ) {

    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to set data of FileHandler to NULL!";
    }
    //Clear the vector and make this string its only element
    m_FileContents.clear();
    m_FileContents.append( newD.trimmed() );

}

void FileHandler::AppendData( const QVector< QString > &newD ) {

    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to append an empty data vector!";
    }

    m_FileContents += newD;

    this->TrimLines();
    
}

void FileHandler::AppendData( const QString &newD ) {

    if ( newD.isEmpty() ) {
        qWarning() << Q_FUNC_INFO << "Trying to append an empty data string!";
    }

    m_FileContents.append( newD.trimmed() );

}

bool FileHandler::operator==( const FileHandler &second ) const {

    //If one file is empty
    if ( ! ( this->HasData() && second.HasData() ) ) {
        return false;
    }

    return ( this->m_FileContents == second.m_FileContents );

}

void FileHandler::operator=( const FileHandler &second ) {

    this->m_FileContents = second.m_FileContents;

}


bool FileHandler::Open( const QString &filename ) {

    QVector< QString > m_FileContents;
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
        m_FileContents.push_back(line);
    }

    return true;

}

bool FileHandler::Close() {

    m_FileContents.clear();
    m_Filename.clear();

    return true;

}

bool FileHandler::Save( const QString &filename ) {

    QFile file( filename );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        qWarning() << Q_FUNC_INFO << "Could not write to file:" << filename;
        return false;
    }
    QTextStream out(&file);
    for ( QVector< QString >::const_iterator s = m_FileContents.begin(); s != m_FileContents.end(); s++ ) {
        out << *s << "\n";
    }

    return true;

}

void FileHandler::TrimLines() {

    for ( QVector< QString >::iterator s = m_FileContents.begin(); s != m_FileContents.end(); s++ ) {
        *s = s->trimmed();
    }

}

void FileHandler::RequestData() {

    emit RawDataV( m_FileContents );

}

void FileHandler::RequestNewData() {

    //Reload the file
    m_FileContents.clear();
    if ( this->Open( m_Filename ) == false ) {
        qWarning() << Q_FUNC_INFO << "Could not reload file:" << m_Filename;
        return;
    }
    
    emit RawDataV( m_FileContents );

}

QVector< QString > FileHandler::GetDataV() const {

    return m_FileContents;

}

#include "filehandler.moc" //Required for CMake, do not remove.