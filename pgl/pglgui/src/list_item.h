#ifndef LIST_ITEM_H
#define LIST_ITEM_H

#include <QString>

#include "blocklist.h"
#include "enums.h"

//extern typedef enum ItemMode { ENABLED_ITEM, DISABLED_ITEM, COMMENT_ITEM } ItemMode;

class Blocklist;

/**
*
* @short Class representing both blocklist entries and comments in the blockcontrol blocklists file.
*
*/

class ListItem {

    public:
        /**
         * Constructor. Creates a ListItem and analyzes the raw line from the configuration file.
         * @param itemRawLine The line from the blockcontrol blocklists file.
         */
        ListItem( const QString &itemRawLine );
        /**
         * Constructor, Creates an empty ListItem.
         */
        ListItem() { mode = ENABLED_ITEM; }
        /**
         * Destructor.
         */
        ~ListItem() { }

        /**
         * The mode of the ListItem.
         * ENABLED_ITEM: Enabled blocklist entry.
         * DISABLED_ITEM: Disabled blocklist entry, line starts with #.
         * COMMENT_ITEM: Not a blocklist. Line starts with # but it's not valid.
         */
        ItemMode mode;
        QString value() const;

        /**
         * Check if this ListItem matches another.
         * This function compares only the locations of the ListItems.
         * @param other The second ListItem.
         * @return True if the ListItems are the same, otherwise false.
         */
        bool operator==( const ListItem &);

        bool isEnabled();
        bool isDisabled();
        bool isBlocklist();
        Blocklist* blocklist() const;

        static bool isValidBlockList(const QString&);
        QString getListName(const QString& );

    private:
        QString mValue;
        Blocklist* mBlocklist;

};

#endif
