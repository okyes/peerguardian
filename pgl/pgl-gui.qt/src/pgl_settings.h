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

#ifndef PGL_SETTINGS_NEW_H
#define PGL_SETTINGS_NEW_H

#include <QString>
#include <QHash>
#include <QStringList>

#include "super_user.h"

#define PGLCMD_DEFAULTS_PATH "/usr/lib/pgl/pglcmd.defaults"

class PglSettings
{

    static QHash<QString, QString> variables;

    public:
        PglSettings();
        ~PglSettings();
        static void loadSettings();
        static QHash<QString, QString> getVariables(){ return variables; }
        static QString getStoredValue(const QString &variable){ return variables[variable]; }
        static QString getValueInLine(QString&);
        static QString getVariableInValue(QString &);
};


#endif

