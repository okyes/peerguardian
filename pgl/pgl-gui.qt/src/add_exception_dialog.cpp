
#include "add_exception_dialog.h" 
#include "utils.h"
#include <QEvent>
#include <QUrl>
#include <QFile>
#include "file_transactions.h"
#include <QCompleter>
#include <QNetworkRequest>


AddExceptionDialog::AddExceptionDialog(QWidget *p, int mode, QList<QTreeWidgetItem*> treeItems) :
	QDialog( p )
{
	setupUi( this );
    
    setPortsFromFile();
    
    //completer for the ports' names
    QCompleter * completer = new QCompleter(ports.keys(), m_addEdit); 
    m_addEdit->setCompleter(completer);
    
    m_validExtensions << ".p2p" << ".zip" << ".7z" << ".gzip" << ".dat";
    
    m_manager = NULL;

    if ( mode == (ADD_MODE | EXCEPTION_MODE) )
    {       
        
        QString help = QObject::tr("Valid Inputs: You can enter an IP Address with");
        help +=  QObject::tr(" or without mask and a port number or name (eg. http, ftp, etc).") + "\n";
        help += QObject::tr("You can enter multiple items separated by ; or ,");
                    
            m_helpLabel->setText(QObject::tr(help.toUtf8 ()));
            m_browseButton->hide();
            
            foreach(QTreeWidgetItem *treeItem, treeItems)
                m_Items.push_back(WhitelistItem(treeItem->text(0), treeItem->text(1), treeItem->text(2)));
                
        connect(m_addEdit, SIGNAL(returnPressed()), this, SLOT(addEntry()));
        connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(addEntry()));
    }
    else if ( mode == (ADD_MODE | BLOCKLIST_MODE) )
    {       
            QString help = QObject::tr("Valid Inputs: You can enter a local path or an address to a valid blocklist.") + "\n";
            help += QObject::tr("You can enter multiple items separated by ; or ,");
            m_helpLabel->setText(help);
            
            setWindowTitle("Add BlockLists");
            groupBox->setTitle("Add one or more BlockLists");
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
}

void AddExceptionDialog::setPortsFromFile()
{
    QStringList fileData = getFileData("/etc/services");
    QStringList elements;
    
    foreach(QString line, fileData)
    {
        if ( line.isEmpty() || line.startsWith("#") )
            continue;
        
        line = line.simplified ();    
        elements = line.split(" ");
        
        //check for alternative names
        if ( elements.size() >= 3 && (! elements[2].startsWith("#")) )
            ports[elements[2]] = elements[1].split("/")[0].toInt();
            
        //qDebug() << line.split(" ")[1].split("/")[0];
        ports[elements[0]] = elements[1].split("/")[0].toInt();
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

bool AddExceptionDialog::isNew(WhitelistItem& whiteItem)
{
    //checks if the new item doesn't already exist
    foreach(WhitelistItem tempItem, m_Items)
        if ( tempItem == whiteItem )
            return false;
            
    return true;
}

QList<WhitelistItem> AddExceptionDialog::getWhitelistItems(QString& param, bool isIp)
{
    QList<WhitelistItem> items;
    QStringList protocols, connections;
    
    protocols = getProtocols(isIp);
    connections = getConnections();
    
    foreach(QString protocol, protocols)
    {
        foreach(QString connection, connections)
        {
            WhitelistItem item = WhitelistItem(param, connection, protocol);
            if ( isNew(item) )
                items.push_back(item);
        }
        
    }

    return items;
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
    QStringList params = getParams(text);
    
    m_notValidTreeWidget->clear();
    m_urls.clear();
    
    foreach(QString param, params)
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

QStringList AddExceptionDialog::getParams(QString& text)
{
    
    QStringList params;
    
    if ( text.indexOf(',') != -1)
        params = text.split(',', QString::SkipEmptyParts);
    else if ( text.indexOf(';') != -1 )
        params = text.split(',', QString::SkipEmptyParts);
    else
        params << text.split(' ', QString::SkipEmptyParts);
        
    return params;
}

bool AddExceptionDialog::isPort(QString & p)
{
    if ( p == QString::number(p.toInt()) )
        return true;
    
    if ( ports.contains(p.toLower()) )
        return true;
    
    return false;
}

void AddExceptionDialog::addEntry()
//Used by exception (whitelist) window
{
    if ( m_addEdit->text().isEmpty() )
        return;
        
    m_NewItems.clear();

    QString text = m_addEdit->text();
    QStringList params, info, values;
    QTreeWidgetItem * item = NULL;
    QVector<QTreeWidgetItem*> items;
    QList<WhitelistItem> newItems;
    bool valid = false;
    QMap<QString, QString> errors;
    QMap<QString, bool> valueIsIp;
    bool ip = false;
    bool port = false;
    QString protocol, comboText;
    
    
    params = getParams(text);
    
    foreach(QString param, params)
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
            errors[param] = "Not a valid IP address nor a Port";
            valueIsIp[param] = ip;
            continue;
        }
        
        newItems += getWhitelistItems(param, ip);
        
        //it could be a valid item but already on main list
        if ( newItems.isEmpty() )
        {
            errors[param] = "This item is already on the main list";
            valueIsIp[param] = ip;
        }    
    }
    
    m_notValidTreeWidget->clear();
       
    if ( ! errors.isEmpty() )
    {
         QStringList connections, protocols;
        
        if ( ! groupBox_2->isVisible() )
            groupBox_2->setVisible(true);
        
        connections = getConnections();
        
        foreach(QString key, errors.keys())
        {
            protocols = getProtocols(valueIsIp[key]);
            foreach(QString protocol, protocols)
            {
                foreach(QString connection, connections)
                {
                    QStringList info; info << key << connection << protocol << errors[key];
                    QTreeWidgetItem *item = new QTreeWidgetItem(m_notValidTreeWidget, info);
                    m_notValidTreeWidget->addTopLevelItem(item); 
                }
            }
        }
    }
    else
    {
        m_NewItems = newItems;
        emit( accept() );
    }        
    
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




