#ifndef WHITELIST_ITEM_P_H
#define WHITELIST_ITEM_P_H

#include <QMap>
#include <QString>
#include <QStringList>

#include "option_p.h"

class WhitelistItemPrivate : public OptionPrivate
{
    public:
        WhitelistItemPrivate();
        ~WhitelistItemPrivate();
        bool operator ==(const WhitelistItemPrivate&);
        bool containsPort(const QString&);

        QStringList values;
        QString connection; //Incoming, Outgoing or Forward
        int type; //Ip or Port
        QString protocol; //TCP, UDP or IP
        QString group;
};

#endif
