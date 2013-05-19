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

#ifndef BLOCKLIST_PRIVATE_H
#define BLOCKLIST_PRIVATE_H

#include <QString>

#include "option_p.h"

/**
*
* @short Class representing a blocklist, either local or remote.
*
*/

class BlocklistPrivate : public OptionPrivate
{

public:
    explicit BlocklistPrivate() {};
    virtual ~BlocklistPrivate() {};

public:
    bool local;
    bool valid;
};

#endif //BLOCKLIST_H

