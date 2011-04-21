
#include "add_exception_dialog.h"
#include "utils.h"
#include <QEvent>
#include <QUrl>
#include <QFile>
#include <QDir>
#include "file_transactions.h"
#include <QCompleter>
#include <QNetworkRequest>
#include <QFileSystemModel>

class WhitelistItem;

AddExceptionDialog::AddExceptionDialog(QWidget *p, int mode, QList<QTreeWidgetItem*> treeItems) :
	QDialog( p )
{
	setupUi( this );

    m_validExtensions << ".p2p" << ".zip" << ".7z" << ".gzip" << ".dat";
    m_manager = NULL;
    QString help;

    if ( mode == (ADD_MODE | EXCEPTION_MODE) )
    {
        bool ok;
        int i;

        setPortsFromFile();
        //completer for the ports' names
        QCompleter * completer = new QCompleter(ports.keys(), m_addEdit);
        m_addEdit->setCompleter(completer);
        m_browseButton->hide();
        QString value;

        help = QObject::tr("Valid Inputs: You can enter an IP Address with");
        help +=  QObject::tr(" or without mask and a port number or name (eg. http, ftp, etc).") + "\n";
        help += QObject::tr("You can enter multiple items separated by spaces, commas or semicolons");

        foreach(QTreeWidgetItem *treeItem, treeItems)
        {
            value = treeItem->text(0);
            WhitelistItem item = WhitelistItem(value, treeItem->text(1), treeItem->text(2));

            foreach(Port port, m_ports)
                if ( port == item )
                    item.addAliases(port.values());

            m_Items.push_back(item);
        }

        connect(m_addEdit, SIGNAL(returnPressed()), this, SLOT(addEntry()));
        connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(addEntry()));
    }
    else if ( mode == (ADD_MODE | BLOCKLIST_MODE) )
    {

            QFileSystemModel * model = new QFileSystemModel(m_addEdit);
            model->setRootPath(QDir::homePath());
            //completer for the ports' names
            QCompleter * completer = new QCompleter(model, m_addEdit);
            m_addEdit->setCompleter(completer);

            QString help = QObject::tr("Valid Inputs: You can enter a local path or an address to a valid blocklist.") + "\n";
            help += QObject::tr("You can enter multiple items separated by spaces, commas or semicolons");

            setWindowTitle("Add Blocklists");
            groupBox->setTitle("Add one or more Blocklists");
            m_ConnectionGroup->hide();
            m_ProtocolGroup->hide();
            m_ipRadio->hide();
            m_portRadio->hide();
            connect(m_browseButton, SIGNAL(clicked()), this, SLOT(selectLocalBlocklist()));

            //to test if the urls are at least working
            m_manager = new QNetworkAccessManager(this);
            connect(m_manager, SIGNAL(finished(QNetworkReply*)),
                 this, SLOT(replyFinished(QNetworkReply*)));

            m_notValidTreeWidget->header()->hideSection(1);
            m_notValidTreeWidget->header()->hideSection(2);


        connect(m_addEdit, SIGNAL(returnPressed()), this, SLOT(addBlocklist()));
        connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(addBlocklist()));

    }
    m_helpLabel->setText(QObject::tr(help.toUtf8 ()));

    /*else if ( mode == (EDIT_MODE | EXCEPTION_MODE) )
    {
            m_exceptionTreeWidget->hide();
            m_helpLabel->hide();
            resize(width(), 300);
    }
    else if ( mode == (EDIT_MODE | BLOCKLIST_MODE))
    {
    }*/

    this->mode = mode;

    resize(width(), height()/2);
    groupBox_2->hide();
}


AddExceptionDialog::~AddExceptionDialog()
{
    if ( m_manager != NULL )
        delete m_manager;

    qDebug() << "~AddExceptionDialog()";
}

Port AddExceptionDialog::getPortFromLine(QString line)
{
    QStringList elements;
    int port_num;
    QString protocol;
    Port port;

    line = line.simplified();

    if ( line.isEmpty() || line.startsWith("#") )
        return Port();

    elements = line.split(" ");

    port_num = elements[1].split("/")[0].toInt();
    protocol = elements[1].split("/")[1];

    port = Port(elements[0], protocol, port_num);

    if ( elements.size() >= 3 && ( ! elements[2].startsWith("#")) )
        port.addAlias(elements[2]);

    return port;
}

void AddExceptionDialog::setPortsFromFile()
{
    QStringList fileData = getFileData("/etc/services");
    QStringList elements;
    int port_num;
    QString protocol;
    Port port1, port2;

    //old way for backwards compatibility
    foreach(QString line, fileData)
    {
        line = line.simplified();

        if ( line.isEmpty() || line.startsWith("#") )
            continue;

        elements = line.split(" ");

        port_num = elements[1].split("/")[0].toInt();
        protocol = elements[1].split("/")[1];

        ports[elements[0]] = port_num;

        //check for alternative names
        if ( elements.size() >= 3 && ( ! elements[2].startsWith("#")) )
            ports[elements[2]] = port_num;
    }

    for ( int i=0; i < fileData.size(); i++ )
    {

        port1 = getPortFromLine(fileData[i]);

        if ( i < fileData.size()-1 )
        {
            //get next line
            port2 = getPortFromLine(fileData[i+1]);

            if ( port1.value() == port2.value() )
            {
                port1.addProtocols(port2.protocols());
                ++i; //ignores next line
            }
        }

        m_ports.push_back(port1);
    }

}

void AddExceptionDialog::replyFinished(QNetworkReply* reply)
{
    if ( reply->error() == QNetworkReply::NoError )
        m_blocklists.push_back(reply->url().toString());
    else
    {
        QStringList info; info << reply->url().toString() << "" << "" << "Not a filepath neither a working URL to a blocklist";
        QTreeWidgetItem *item = new QTreeWidgetItem(m_notValidTreeWidget, info);
        qDebug() << item->text(4);
        m_notValidTreeWidget->addTopLevelItem(item);

        if ( ! groupBox_2->isVisible() )
        {
            groupBox_2->setVisible(true);
            m_notValidTreeWidget->setMinimumSize(m_notValidTreeWidget->width(), m_notValidTreeWidget->topLevelItemCount()*17 + 17);
        }
    }

    if ( m_urls.contains(reply->url()))
        m_urls.removeOne(reply->url());

    //If we searched all urls and there is no errors, emit the accept signal
    if ( m_urls.size() == 0 && m_notValidTreeWidget->topLevelItemCount() == 0)
        emit(accept());
}

void AddExceptionDialog::selectLocalBlocklist()
{
    QString filter;
    filter += "All Supported files  (*.p2p *.gz *.7z *.zip *.dat );;";
    filter += "P2P (*.p2p);;Zip (*.zip);; 7z (*.7z);;Gzip (*.gz);;Dat (*.dat)";

    QStringList files = selectFiles(this, filter);

    QString text = m_addEdit->text();
    text += files.join(QString(" ; "));
    if ( files.size() == 1 )
        text += "; ";
    m_addEdit->setText(text);
}

bool AddExceptionDialog::isValidException(QString& text)
{
    if ( isPort( text ) )
        return true;
    else if( isValidIp(text) )
        return true;

    return false;
}

bool AddExceptionDialog::isValidBlocklist(QString& text)
{
    //Don't check for file extension, for now, although this may change in the future.

    return false;
}


QStringList AddExceptionDialog::getConnections()
{
    QStringList connections;

    if ( m_OutCheck->checkState() == Qt::Checked )
        connections << m_OutCheck->text();

    if ( m_InCheck->checkState() == Qt::Checked )
        connections << m_InCheck->text();

    if ( m_FwdCheck->checkState() == Qt::Checked )
        connections << m_FwdCheck->text();

    return connections;
}

QStringList AddExceptionDialog::getProtocols(bool isIp)
{
    QStringList protocols;

    if ( isIp )
        protocols << "IP";
    else
    {
        if ( m_TcpCheck->checkState() ==  Qt::Checked )
            protocols << m_TcpCheck->text();
        if ( m_UdpCheck->checkState() ==  Qt::Checked )
            protocols << m_UdpCheck->text();
    }

    return protocols;
}

void AddExceptionDialog::addBlocklist()
{
    QString text = m_addEdit->text();
    QStringList values = getParams(text);

    m_notValidTreeWidget->clear();
    m_urls.clear();

    foreach(QString param, values)
    {
        param = param.trimmed();

        if ( param.isEmpty() )
            continue;

        if ( QFile::exists(param) )
            m_blocklists.push_back(param);
        else
        {
            //we save each possible url in a QList to test it later on
            QUrl url = QUrl(param);
            m_urls.push_back(url);
        }
    }

    if ( m_urls.isEmpty() )
        emit(accept());
    else
        //test each url to see if, at least, is a working URL
        foreach( QUrl url, m_urls )
            m_manager->get(QNetworkRequest(url));
}

QStringList AddExceptionDialog::getParams(const QString& text)
{

    QStringList values;

    if ( text.indexOf(',') != -1)
        values = text.split(',', QString::SkipEmptyParts);
    else if ( text.indexOf(';') != -1 )
        values = text.split(';', QString::SkipEmptyParts);
    else
        values = text.split(' ', QString::SkipEmptyParts);

    return values;
}

bool AddExceptionDialog::isPort(QString & p)
{
    if ( p == QString::number(p.toInt()) )
        return true;

    if ( ports.contains(p.toLower()) )
        return true;

    return false;
}


bool AddExceptionDialog::isValidWhitelistItem(WhitelistItem& whiteItem, QString& reason)
{
    reason = "";

    //checks if the new item doesn't already exist
    foreach(WhitelistItem tempItem, m_Items)
        if ( tempItem == whiteItem )
        {
            qDebug() << tempItem.value() << whiteItem.value();
            reason = QObject::tr("It's already added");
            return false;
        }

    foreach(Port port, m_ports)
        if ( port == whiteItem )
            if ( ! port.hasProtocol(whiteItem.protocol()) )
            {
                reason += whiteItem.value();
                reason += QObject::tr(" doesn't work over ") + whiteItem.protocol();
                return false;
            }

    return true;
}

void AddExceptionDialog::setWhitelistItems(QString& value, bool isIp)
{
    QList<WhitelistItem> items;
    QStringList protocols, connections;

    protocols = getProtocols(isIp);
    connections = getConnections();

    QString reason;

    foreach(QString protocol, protocols)
    {
        foreach(QString connection, connections)
        {
            WhitelistItem item = WhitelistItem(value, connection, protocol);

            if ( isValidWhitelistItem(item, reason) )
                m_validItems << item;
            else
            {
                m_invalidItems << item;
                reasons << reason;
            }
        }

    }
}

void AddExceptionDialog::addEntry()
//Used by exception (whitelist) window
{
    if ( m_addEdit->text().isEmpty() )
        return;

    m_invalidItems.clear();
    m_validItems.clear();
    reasons.clear();

    QStringList values, info;
    bool valid = false;
    bool ip = false;
    bool port = false;
    QStringList unrecognizedValues;


    values = getParams(m_addEdit->text());

    foreach(QString param, values)
    {
        param = param.trimmed();

        if ( param.isEmpty() )
            continue;

        if ( isPort(param) )
            ip = false;
        else if ( isValidIp(param ) )
            ip = true;
        else
        {
            unrecognizedValues << param;
            continue;
        }

        setWhitelistItems(param, ip);
    }

    m_notValidTreeWidget->clear();

    if ( ! unrecognizedValues.isEmpty() || ! m_invalidItems.isEmpty() )
    {
         QStringList connections, protocols;

        if ( ! groupBox_2->isVisible() )
            groupBox_2->setVisible(true);

        foreach(QString value, unrecognizedValues)
        {
            QStringList info; info << value << "ANY" << "ANY" << "Not a valid IP nor a Port";
            QTreeWidgetItem *item = new QTreeWidgetItem(m_notValidTreeWidget, info);
            m_notValidTreeWidget->addTopLevelItem(item);
        }

        for(int i=0; i < m_invalidItems.size(); i++)
        {
            WhitelistItem whiteItem = m_invalidItems[i];
            QStringList info; info << whiteItem.value() << whiteItem.connection() << whiteItem.protocol() << reasons[i];
            QTreeWidgetItem *item = new QTreeWidgetItem(m_notValidTreeWidget, info);
            m_notValidTreeWidget->addTopLevelItem(item);
        }

    }
    else
        emit( accept() );

}


int AddExceptionDialog::getPort(const QString& p)
{
    if ( ports.contains(p.toUpper()) )
        return ports[p.toUpper()];

    if ( p == QString::number(p.toInt()) )
        return p.toInt();

    return -1;
}

void AddExceptionDialog::keyPressEvent ( QKeyEvent * e )
{
    // have tried many different things in here
    qDebug( "Dialog key pressed %d", e->key() );

    if (e->key() == Qt::Key_Escape)
        QDialog::keyPressEvent (e);

}




