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
    ENABLED,
    DISABLED,
    INVALID
};


class WhitelistItem
{

    QString m_Value;
    QStringList m_values;
    QString m_Connection; //Incoming, Outgoing or Forward
    int m_Type; //Ip or Port
    QString m_Protocol; //TCP, UDP or IP
    QString m_Group;
    bool m_Enabled;
    bool m_Valid;

    public:
        WhitelistItem();
        WhitelistItem(const QString&, const QString&, const QString&, int type=ENABLED);
        WhitelistItem(const QString&, const QString&, int type=ENABLED);
        ~WhitelistItem(){};
        QString value() { return m_Value; }
        QStringList values() { return m_values; }
        bool valid () { return m_Valid; }
        void setValid (bool valid) { m_Valid = valid; }
        QString connection() const { return m_Connection; }
        int type() { return m_Type; }
        QString protocol() const { return m_Protocol; }
        QString group() { return m_Group; }
        bool isEnabled(){ return m_Enabled; }
        QStringList values() const { return m_values; }
        void addAlias(const QString &);
        void addAliases(const QStringList& );
        QString getTypeAsString();
        QStringList getAsStringList(){ return QStringList() << m_Value << getTypeAsString(); }
        bool operator==(const WhitelistItem & otherItem);

};



class Port
{
    int m_number;
    QStringList m_protocols;
    QStringList m_values;

    public:
        Port();
        Port(const Port& other);
        Port(QString desig, QString prot, int n=0);
        ~Port(){};
        void addProtocols(const QStringList&);
        void addAlias(const QString&);
        int number() const { return m_number;}
        QStringList values() const { return m_values; }
        QString value();
        QStringList protocols() const { return m_protocols; }
        Port& operator=(const Port& other);
        bool hasProtocol(const QString& protocol) { return m_protocols.contains(protocol, Qt::CaseInsensitive); }
        bool operator==(WhitelistItem& item);
        bool operator==(const Port& );
};

class PglWhitelist
{
    QList<WhitelistItem> m_WhitelistedItems;
    QString m_WhitelistFile;
    QMap<QString, int> m_Group;
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
        QStringList updatePglcmdConf(QList<QTreeWidgetItem*>);
        QList<WhitelistItem> getWhitelistItems(QList<QTreeWidgetItem*>);
        QMap<QString, QStringList> getEnabledWhitelistedItems() { return m_WhitelistEnabled; }
        QMap<QString, QStringList> getDisabledWhitelistedItems(){ return m_WhitelistDisabled; }
        QString getTypeAsString(QString&);
        QString getGroup(QStringList&);
        QStringList updateWhitelistFile();
        void updateSettings(const QList<QTreeWidgetItem*>& treeItems, int firstAddedItemPos=0, bool updateAll=true);
        QString getWhitelistFile(){ return m_WhitelistFile; };
        QString getProtocol(QString& key);
        QString translateConnection(const QString&);
        QStringList getDirections(const QString& chain);
        QStringList getCommands(QStringList items, QStringList connections, QStringList protocols, QList<bool> allows);
        void addTreeWidgetItemToWhitelist(QTreeWidgetItem* item);
        void load();
};

#endif
