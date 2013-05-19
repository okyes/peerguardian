#include "list_item.h"

#include <QFile>
#include <QFileInfo>
#include <QStringList>

ListItem::ListItem( const QString &itemRawLine ) {

    QString itemLine = itemRawLine.trimmed();

    mValue = itemRawLine;
    mode = COMMENT_ITEM;
    mBlocklist = 0;

    if ( itemLine.isEmpty() ) {
        mode = COMMENT_ITEM;
    }
    else if (itemLine.startsWith('#')){
        if ( Blocklist::isValid(itemLine) )
        {
            mode = DISABLED_ITEM;
            mValue = itemLine;
            mBlocklist = new Blocklist(itemLine, true, false);
        }
        else
            mode = COMMENT_ITEM;
    }
    else if(Blocklist::isValid(itemLine)){
        mode = ENABLED_ITEM;
        mValue = itemLine;
        mBlocklist = new Blocklist(itemLine, true);
    }
}

bool ListItem::isDisabled()
{
    return (mode == DISABLED_ITEM);
}

bool ListItem::isEnabled()
{
    return (mode == ENABLED_ITEM);
}

QString ListItem::value() const
{
    return mValue;
}

bool ListItem::operator==( const ListItem &other )
{
    if (mBlocklist && other.blocklist())
        return (mBlocklist->location() == other.blocklist()->location());
    return (value() == other.value());
}

bool ListItem::isBlocklist()
{
    if (mBlocklist)
        return true;
    return false;
}

Blocklist* ListItem::blocklist() const
{
    return mBlocklist;
}
