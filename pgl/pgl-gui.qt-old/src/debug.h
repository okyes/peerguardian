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


/**
 * The default format of the debug messages of the program.
 */
#define DEBUG_MSG qDebug() << Q_FUNC_INFO << "->"
#define WARN_MSG qWarning() << Q_FUNC_INFO << "->"

#include <QDebug>

/**
 * Sets a custom format for the debugging output produced by qDebug(), qWarning(), qCritical and qFatal().
 *
 * This function should be installed as a message handler using qInstallMsgHandler().
 * @param type The type of the output message.
 * @param msg The message string.
 */
void customOutput( QtMsgType type, const char *msg );
