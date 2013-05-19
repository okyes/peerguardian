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

#include "whitelist_item_p.h"
#include "whitelist_item.h"
#include "utils.h"
#include "pgl_settings.h"
#include "file_transactions.h"

#include <QDebug>

WhitelistItemPrivate::WhitelistItemPrivate()
{
}

WhitelistItemPrivate::~WhitelistItemPrivate()
{
    qDebug() << "~WhitelistItemPrivate()";
}

bool WhitelistItemPrivate::operator==(const WhitelistItemPrivate& other)
{
    if ( protocol != other.protocol )
        return false;

    if ( connection != other.connection )
        return false;

    foreach(const QString& value, values)
        foreach(const QString& value2, other.values)
            if (value == value2)
                return true;

    //match port and port ranges
    return containsPort(other.value.toString());
}

bool WhitelistItemPrivate::containsPort(const QString& value)
{
    bool ok = false;
    int port = value.toInt(&ok);
    int port2 = 0, start = 0, end = 0;
    QStringList range;

    if (ok) {
        port2 = value.toInt(&ok);
        if (ok) {
            return port == port2;
        }
        else if (value.contains(":")){
            range = value.split(":");
            if (range.count() > 0) {
                start = range.first().toInt();
                end = range.last().toInt(&ok);
                if (! ok) end = 65535;
                if (port >= start && port <= end)
                    return true;
            }
        }
    }
    else if (value.contains(":")) {
        range = value.split(":");
        if (range.count() > 0) {
            start = range.first().toInt();
            end = range.last().toInt(&ok);
            if (! ok) end = 65535;
        }

        port2 = value.toInt(&ok);

        if (ok) {
            if (port2 >= start && port2 <= end)
                return true;
        }
        else if (value.contains(":")){
            range = value.split(":");
            if (range.count() > 0) {
                int start2 = range.first().toInt();
                int end2 = range.last().toInt(&ok);
                if (! ok) end = 65535;
                if (start2 >= start && end2 <= end)
                    return true;
            }
        }
    }

    return false;
}

WhitelistItem::WhitelistItem(const QString& value, const QString& connType, const QString& prot, bool active, bool enabled) :
    Option(),
    d_ptr(new WhitelistItemPrivate),
    d_active_ptr(new WhitelistItemPrivate)
{
    setData(d_ptr);
    setActiveData(d_active_ptr);

    setValue(value);
    d_ptr->values << value;
    d_ptr->protocol = prot;
    d_ptr->connection = connType;
    d_ptr->removed = false;
    setEnabled(enabled);

    if (active)
        *d_active_ptr = *d_ptr;
}

WhitelistItem::WhitelistItem(const WhitelistItem& item)
{
    d_ptr = 0;
    d_active_ptr = 0;
    *this = item;
}


WhitelistItem::~WhitelistItem()
{
    //pointers will be deleted by Option destructor
    d_ptr = 0;
    d_active_ptr = 0;
}

QString WhitelistItem::value() const
{
    return d_ptr->value.toString();
}

QStringList WhitelistItem::values() const
{
    return d_ptr->values;
}

QString WhitelistItem::connection() const
{
    return d_ptr->connection;
}

int WhitelistItem::type()
{
    return d_ptr->type;
}

QString WhitelistItem::protocol() const
{
    return d_ptr->protocol;
}

QString WhitelistItem::group() const
{
    return d_ptr->group;
}

bool WhitelistItem::operator==(const WhitelistItem& other)
{
    return (*d_ptr == *(other.d_ptr));
}

void WhitelistItem::addAlias(const QString & alias )
{
    if ( alias.isEmpty() )
        return;

    d_ptr->values << alias;
    d_active_ptr->values << alias;
}

void WhitelistItem::addAliases(const QStringList & aliases)
{
    foreach(QString alias, aliases)
        if ( ! d_ptr->values.contains(alias, Qt::CaseInsensitive) ) {
            d_ptr->values << alias;
            d_active_ptr->values << alias;
        }
}

WhitelistItem& WhitelistItem::operator=(const WhitelistItem& other)
{
    if (other.d_ptr && other.d_active_ptr) {
        if (! d_ptr)
            d_ptr = new WhitelistItemPrivate;
        if (! d_active_ptr)
            d_active_ptr = new WhitelistItemPrivate;

        *(d_ptr) = *(other.d_ptr);
        *(d_active_ptr) = *(other.d_active_ptr);

        //set parent Option to point to same data
        setData(d_ptr);
        setActiveData(d_active_ptr);
    }

    return *this;
}


