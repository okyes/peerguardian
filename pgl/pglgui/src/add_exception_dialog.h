
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
#include <QHash>

#include "ui_add_exception.h"
#include "pgl_whitelist.h"
#include "utils.h"
#include "blocklist.h"
#include "pgl_lists.h"
#include "pglcore.h"
#include "port.h"

enum openMode { ADD_MODE=0x000,
            EDIT_MODE,
            BLOCKLIST_MODE,
            EXCEPTION_MODE
};

class PglCore;

class AddExceptionDialog : public QDialog, private Ui::AddExceptionDialog {

	Q_OBJECT

    QList<Port> mPorts;
    QHash<QString, int> mPortsPair;
    QList<WhitelistItem> m_Items;
    QList<WhitelistItem> m_validItems;
    QList<WhitelistItem> m_invalidItems;
    QStringList reasons;
    int mode;
    QStringList m_validExtensions;
    QStringList m_blocklists;
    PglCore* mPglCore;
	
	public:
        AddExceptionDialog(QWidget *p = 0, int mode=0);
        AddExceptionDialog(QWidget *p, int mode, PglCore*);
        ~AddExceptionDialog();
        QStringList getBlocklistInfo(QString&);
        QStringList getExceptionInfo(QString&);
        QVector<QTreeWidgetItem*> getTreeItems(QTreeWidget *);
        bool isValidException(QString&);
        QList<WhitelistItem> getItems(){ return m_validItems; }
        QStringList getBlocklists() { return m_blocklists; }
        bool isValidWhitelistItem(WhitelistItem& whiteItem, QString& reason);
        void setWhitelistItems(QString&, bool);
        QStringList getConnections();
        QStringList getProtocols(bool);
        QStringList getParams(const QString& text);

    public slots:
        void addEntry();
        void selectLocalBlocklist();
        void addBlocklist();

    protected:
        void keyPressEvent ( QKeyEvent * e );

};


#endif
