 
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

#include "ui_add_exception.h"
#include "pgl_whitelist.h"

typedef enum openMode { ADD_MODE=0x000,
                EDIT_MODE,
                BLOCKLIST_MODE,
                EXCEPTION_MODE
            };
        
            

class AddExceptionDialog : public QDialog, private Ui::AddExceptionDialog {

	Q_OBJECT
    
    QMap<QString, int> ports;
    QList<WhitelistItem> m_NewItems;
    QList<WhitelistItem> m_Items;
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
        QList<WhitelistItem> getItems(){ return m_NewItems; }
        QStringList getBlocklists() { return m_blocklists; }
        bool isNew(WhitelistItem& whiteItem);
        QList<WhitelistItem> getWhitelistItems(QString&, bool);
        QStringList getConnections();
        QStringList getProtocols(bool);
        QStringList getParams(QString& text);
        void setPortsFromFile();
        bool isPort(QString &);
        
    public slots:
        void addEntry();
        void selectLocalBlocklist();
        void replyFinished(QNetworkReply* reply);
        void addBlocklist();
    
    protected:
        void keyPressEvent ( QKeyEvent * e );
        
};


#endif
