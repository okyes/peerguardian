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

#ifndef RAWDATA_H
#define RAWDATA_H

#include <QObject>

class QString;
class QVector;

/**
 * @brief An Abstract class representing raw data which will be processed by the program.
 *
 * This is used as base class for classes which read and write data to processes, files and dbus.
 * Due to this fact, this class essentially does nothing.
 *
 */

class RawData : public QObject {

    Q_OBJECT

    public:
        /**
         * Constructor.
         *
         * As this is an abstract class, this function does nothing.
         * @param parent The QObject parent of this object. 
         */
        RawData( QObject *parent = 0 );
        /**
         * Destructor
         * 
         * Has to be virtual so that the derived classes are destroyed sucessfully.
         */
        virtual ~RawData();


        /**
         * Checks if there are any data to give.
         * 
         * @return True if there are any data available, otherwise false.
         */
        virtual bool HasData() const = 0;
        /**
         * Gives the raw data to the caller.
         *
         * @return A QString with the data.
         */
        virtual const QString GetDataS() const;
        /**
        * Gives the raw data to the caller.
        *
        * @return A QVector of QStrings with the data.
        */
        virtual const QVector< QString > GetDataV() const;


    public slots:
        /**
        * Opens a Data Source.
        *
        * Pure virtual function, does nothing.
        * @param name The name of the file/process/dbus channel.
        * @return True if the source was opened sucessfully, otherwise false.
        */
        virtual bool Open( const QString &name ) = 0;
        /**
        * Closes the data source being used.
        *
        * Pure virtual function, does nothing.
        * @return True if the source was closed sucessfully, otherwise false.
        */
        virtual bool Close() = 0;
        /**
        * Emit the appropriate signal and send the data to the caller.
        */
        virtual void RequestData() = 0;       
        /**
         * This is essentially the same as RequestData but it emits a singal only if the data have not been read before.
         * 
         * @see RequestData()
         */
        virtual void RequestNewData() = 0;

};
        
#endif //RAWDATA_H
