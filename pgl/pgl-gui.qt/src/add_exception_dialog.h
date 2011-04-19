 
#ifndef ADD_EXCEPTION_DIALOG_H
#define ADD_EXCEPTION_DIALOG_H

#include <QDialog>
#include <QWidget>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QProcess>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QList> 
#include <QDebug>
#include <QAbstractButton>
#include <QKeyEvent>
#include <QObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHash>

#include "ui_add_exception.h"
#include "pgl_whitelist.h"



typedef enum openMode { ADD_MODE=0x000,
                EDIT_MODE,
                BLOCKLIST_MODE,
                EXCEPTION_MODE
            };


/*class Port
{
    QString m_desig;
    int m_number;
    QString m_protocol;
    
    public:
        Port(){};
        Port(QString desig, QString prot, int n=0) { m_desig = desig; m_protocol = prot; m_number = n; }
        Port(int n) { m_number = n; }
        ~Port(){};
        
        int number(){ return m_number;}
        QString desig(){ return m_desig; }
        QString protocol() { return m_protocol; }
        void addAlias(const QString& alias) {}
};*/
            

class AddExceptionDialog : public QDialog, private Ui::AddExceptionDialog {

	Q_OBJECT
    
    QHash<QString, int> ports;
    QList<Port> m_ports;
    QList<WhitelistItem> m_Items;
    QList<WhitelistItem> m_validItems;
    QList<WhitelistItem> m_invalidItems;
    QStringList reasons;
    int mode;
    QStringList m_validExtensions;
    QNetworkAccessManager *m_manager;
    QList<QUrl> m_urls;
    QStringList m_blocklists;
    
	
	public:
        AddExceptionDialog(QWidget *p = 0, int mode=0);
        AddExceptionDialog(QWidget *p, int mode, QList<QTreeWidgetItem*> treeItems);
        ~AddExceptionDialog();
        int getPort(const QString&);
        QStringList getBlocklistInfo(QString&);
        QStringList getExceptionInfo(QString&);
        QVector<QTreeWidgetItem*> getTreeItems(QTreeWidget *);
        bool isValidBlocklist(QString&);
        bool isValidException(QString&);
        QList<WhitelistItem> getItems(){ return m_validItems; }
        QStringList getBlocklists() { return m_blocklists; }
        bool isValidWhitelistItem(WhitelistItem& whiteItem, QString& reason);
        void setWhitelistItems(QString&, bool);
        QStringList getConnections();
        QStringList getProtocols(bool);
        QStringList getParams(const QString& text);
        void setPortsFromFile();
        bool isPort(QString &);
        Port getPortFromLine(QString);
        
    public slots:
        void addEntry();
        void selectLocalBlocklist();
        void replyFinished(QNetworkReply* reply);
        void addBlocklist();
    
    protected:
        void keyPressEvent ( QKeyEvent * e );
        
};


#endif
