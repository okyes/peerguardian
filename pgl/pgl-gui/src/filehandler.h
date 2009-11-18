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

#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <QObject>

#include <QVector>
#include <QString>

/**
 * @brief A class which deals with raw text files.
 *
 * FileHandler can load, edit and save files. It can also be used to emit signals when file change and reload them.
 * 
 */

class FileHandler : public QObject {


    Q_OBJECT

    public:
        /**
         * Default constructor. Can be called with no arguments.
         *
         * @param autoE True if you want the signal with the file data to be emitted every time the file is (re)loaded.
         * @param parent The parent of this object.
         */
        FileHandler( QObject *parent = 0 );
        /**
         * Alternative constructor. Creates a FileHandler object and loads the file requested.
         * 
         * @param filename The name of the file which will be opened.
         * @param autoE True if you want the signal with the file data to be emitted every time the file is (re)loaded.
         * @param parent The parent of this object.
         */
        FileHandler( const QString &filename, const bool &autoE = false, QObject* parent = 0 );
        /**
         * Destructor.
         * 
         * Destroys the FileHandler object. Does NOT save the file.
         */
        ~FileHandler();
        /**
         * The name of the file currently loaded.
         *
         * @return The filename.
         */
        QString getFilename() const;
        /**
         * Checks if there are any data currently loaded.
         * 
         * @return True if the file contents are not empty.
         */
        bool hasData() const;
        /**
        * Gives the loaded file data to the caller.
        *
        * @return A QString with the data.
        */
        QString getDataS() const;
        /**
        * Gives the loaded file data to the caller.
        *
        * @return A QVector of QStrings with the data.
        */
        QVector< QString > getDataV() const;
        /**
         * Set the file data to be the contets of the QVector< QString > given.
         * 
         * @param newD The new file data.
         */
        void setData( const QVector< QString > &newD );
        void setData( const QString &newD );
        /**
         * Selects whether the file will emit a signal with its data when it's (re)loaded or not.
         * If this is set to false, you will have to call RequestData() for the signal to be emitted.
         * 
         * When a new object is constructed, this is set to false by default.
         * @param autoE True if you want the signal to be emitted automatically, otherwise false
         */
         void setAutoEmit( const bool &autoE );
        /**
         * Append new data to the contents of the file already loaded.
         * 
         * @param newD The data to be appeneded.
         */
        void appendData( const QVector< QString > &newD );
        void appendData( const QString &newD );
        /**
         * Compares two FileHandler objects.
         * 
         * Does NOT compare file names.
         * @param second The FileHandler which *this will be compared to.
         * @return True if the objects contain the same data, otherwise false.
         */
        bool operator==( const FileHandler &second ) const;
        /**
         * Copies the data from another FileHandler object to this one.
         * 
         * @param second The FileHandler from which the data will be copied.
         */
        void operator=( const FileHandler &second );
        
    public slots:
        /**
        * Opens a File and loads its data.
        *
        * @param filename The name of the file.
        * @return True if the file was opened sucessfully, otherwise false.
        */
        bool load( const QString &filename );
        /**
         * Reloads the file currently which is currently open.
         * 
         * @return True if the file was opened sucessfully, otherwise false.
         */
        bool reload();
        /**
        * Closes the data file being used, discarding its data.
        * 
        * 
        */
        void discard();
        /**
        * Saves the data into a file.
        *
        * @param filename The name of the file where the data will be saved. Leave this empty to save to the same file.
        * @return True if the data were sucessfully saved.
        */
        virtual bool save( const QString &filename = QString() );
        /**
        * Emit the loaded file data using fileDataV() signal
        *
        * @see RequestData()
        */
        void requestData();

    signals:
        void fileDataV( const QVector< QString > &newD );


    private:
        /**
         * Trim the lines of the vector using QString::trimmed()
         */
        void trimLines();
        QString m_Filename;
        QVector< QString > m_FileContents;
        bool m_AutoEmitSignal;

};

#endif //FILEHANDLER_H
