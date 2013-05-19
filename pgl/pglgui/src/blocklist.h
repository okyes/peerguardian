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

#ifndef BLOCKLIST_H
#define BLOCKLIST_H

#include "settings.h"
#include "option.h"
#include "blocklist_p.h"

/**
*
* @short Class representing a blocklist, either local or remote.
*
*/

class BlocklistPrivate;

class Blocklist : public Option
{

public:
    Blocklist(const QString&, bool active=false, bool enabled=true);
    Blocklist(const Blocklist&);
    virtual ~Blocklist();

    bool isLocal() const;
    bool isValid() const;
    bool exists() const;
    QString location() const;
    static bool isValid(const QString&);

protected:
    QString parseName(const QString&);
    QString parseUrl(const QString&);

private:
    BlocklistPrivate *d_active_ptr;
    BlocklistPrivate *d_ptr;

};



#endif //BLOCKLIST_H

