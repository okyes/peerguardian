 
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
        bool isNew(WhitelistItem& whiteItem);
        QList<WhitelistItem> getWhitelistItems(QString&, bool);
        QStringList getConnections();
        QStringList getProtocols(bool);
        
    public slots:
        void addEntry();
        void addBlocklist();
    
    protected:
        void keyPressEvent ( QKeyEvent * e );
        bool event ( QEvent * event );
        
};


#endif
