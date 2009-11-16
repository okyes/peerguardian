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

#ifndef ABSTRACTHANDLER_H
#define ABSTRACTHANDLER_H

class QString;

/**
 * @brief Simple class intented to be a base for all high level data handlers(log, settings etc).
 * 
 * Its main function is to set the path of the data file after checking for its validity.
 */

class AbstractHandler {

    public:
        /**
         * Default constructor, does nothing.
         */
        AbstractHandler();
        /**
         * Alternative constructor.
         * 
         * Creates an AbstractHandler object and uses setFilePath() to set its path.
         * @param path The path to the data file.
         */
        AbstractHandler( const QString &path );
        /**
         * Destructor, does nothing.
         */
        ~AbstractHandler();
        /**
         * Sets the path to the data file.
         * 
         * This function checks if the path is empty and if the file exists.
         * If any of the above is false, the function prints the appropriate error message.
         * @param path The path to the data file.
         */
        void setFilePath( const QString &path );
        /**
         * Indicates if the object is intiallized correctly.
         * 
         * @return True if the current file path set is valid, otherwise false.
         */
        bool isWorking() const;

    private:
        QString m_FilePath;

};

#endif //ABSTRACTHANDLER_H
