#ifndef PGL_WHITELIST_H
#define PGL_WHITELIST_H

#include <QMultiMap>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QList>
#include <QTreeWidgetItem>

#define WHITE_IP_IN "WHITE_IP_IN"
#define WHITE_IP_OUT "WHITE_IP_OUT"
#define WHITE_IP_FWD "WHITE_IP_FWD"
#define WHITE_TCP_IN "WHITE_TCP_IN"
#define WHITE_UDP_IN "WHITE_UDP_IN"
#define WHITE_TCP_OUT "WHITE_TCP_OUT"
#define WHITE_UDP_OUT "WHITE_UDP_OUT"
#define WHITE_TCP_FWD "WHITE_TCP_FWD"
#define WHITE_UDP_FWD "WHITE_UDP_FWD"


typedef enum { 
    TYPE_INCOMING,
    TYPE_OUTGOING,
	TYPE_FORWARD };

typedef enum {
    IP,
    PORT,
    INVALID
};

typedef enum {
    TCP,
    UDP
};


class WhitelistItem
{
    
    QString m_Value;
    int m_Connection; //Incoming, Outgoing or Forward
    int m_Type; //Ip or Port
    int m_Transport; //TCP or UDP
    QString m_Group;
    bool m_Enabled;

    public:
        WhitelistItem();
        WhitelistItem(QString&, int, bool, QString&);
        ~WhitelistItem(){};
        QString value() { return m_Value; }
        int connection() { return m_Connection; }
        int type() { return m_Type; }
        int transport() { return m_Transport; }
        QString group() { return m_Group; }
        bool isEnabled(){ return m_Enabled; }
        QString getConnectionAsString();
        QString getTypeAsString();
        QStringList getAsStringList(){ return QStringList() << m_Value << getTypeAsString(); }
        
};

class PglWhitelist 
{
    QList<WhitelistItem> m_WhitelistedItems;
    QString m_WhitelistFile;
    QMap<QString, int> m_WhitelistConnection;
    QSettings * m_Settings;
    QMap<QString, QStringList> m_WhitelistEnabled;
    QMap<QString, QStringList> m_WhitelistDisabled;
    
	public:
		/**
		 * Constructor. Creates an emtpy PglWhitelist object with no data loaded.
		 */
		PglWhitelist(QSettings *);
		/**
		 * Destructor.
		 */
		~PglWhitelist() { }
        
        QList<WhitelistItem> getWhitelistedItems(){ return m_WhitelistedItems; }
        void loadDisabledItems(QSettings*);
        void disableItems(QList<QTreeWidgetItem*>);
        QList<WhitelistItem> getWhitelistItems(QList<QTreeWidgetItem*>);
        QList<QStringList> getEnabledWhitelistedItems() { return m_WhitelistEnabled.values(); }
        QList<QStringList> getDisabledWhitelistedItems(){ return m_WhitelistDisabled.values(); }
        
};

#endif
