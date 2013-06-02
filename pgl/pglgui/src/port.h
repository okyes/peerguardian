/***************************************************************************
 *   Copyright (C) 2013 by Carlos Pais <freemind@lavabit.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
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

#ifndef PORT_H
#define PORT_H

#include <QString>
#include <QStringList>

class Port
{
    int mNumber;
    QStringList mProtocols;
    QStringList mNames;

    public:
        Port();
        Port(const Port& other);
        Port(const QString& desig, const QString& prot, int n=0);
        virtual ~Port(){};
        void addProtocols(const QStringList&);
        void addName(const QString&);
        bool containAlias(const QString&);
        int number() const;
        QStringList names() const;
        QString name() const;
        bool containsName(const QString&) const;
        QStringList protocols() const;
        //Port& operator=(const Port& other);
        bool hasProtocol(const QString&) const;
        //bool operator==(WhitelistItem& item);
        bool operator==(const Port& );
};

#endif
