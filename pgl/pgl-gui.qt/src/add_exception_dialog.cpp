
#include "add_exception_dialog.h" 
#include "utils.h"
#include <QEvent>


AddExceptionDialog::AddExceptionDialog(QWidget *p, int mode, QList<QTreeWidgetItem*> treeItems) :
	QDialog( p )
{
	setupUi( this );
    
    ports["HTTP"] = 80;
    ports["HTTPS"] = 443;
    ports["FTP"] = 21;
    ports["POP"] = 110;
    ports["SMTP"] = 587;
    ports["IMAP"] = 143;
    ports["SSH"] = 22;

    if ( mode == (ADD_MODE | EXCEPTION_MODE) )
    {       
        
        QString help = "Valid Inputs: You can enter an IP Address with";
        help +=  " or without mask and a port number or name (eg. http, ftp, etc).\n";
        help += "You can enter multiple items separated by ';' or ',' .";
                    
            m_helpLabel->setText(QObject::tr(help.toUtf8 ()));
            m_browseButton->hide();
            
            foreach(QTreeWidgetItem *treeItem, treeItems)
                m_Items.push_back(WhitelistItem(treeItem->text(0), treeItem->text(1), treeItem->text(2)));
    }
    else if ( mode == (ADD_MODE | BLOCKLIST_MODE) )
    {
            m_helpLabel->setText(QObject::tr("Valid Inputs: You can enter a local path or an address to a valid blocklist."));
            setWindowTitle("Add BlockLists");
            groupBox->setTitle("Add one or more BlockLists");
            connect(m_browseButton, SIGNAL(clicked()), this, SLOT(addBlocklist()));
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
    connect(m_addEdit, SIGNAL(returnPressed()), this, SLOT(addEntry()));
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(addEntry()));
    
    resize(width(), height()/2);
    groupBox_2->hide();
}

AddExceptionDialog::~AddExceptionDialog()
{
}

bool AddExceptionDialog::event( QEvent * event )
{
    if( event->type() == QEvent::Close )
        qDebug() << "Close event!";
    
    /*if ( event->type() == QEvent::Hide || event->type() == QEvent::HideToParent || event->type() == QEvent::WindowDeactivate)
    {
        event->ignore();
        qDebug() << event->type();
        return true;
    }*/
    //qDebug() << event->type();
    QDialog::event(event);
    
    
}


void AddExceptionDialog::addBlocklist()
{
    QStringList files = selectFiles(this);
    m_addEdit->setText(files.join(QString(" , ")));
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
    //FIXME: Needs to check the file extension
    if ( QFile::exists(text) )
        return true;
    //FIXME: Needs to test if is an URL and if it's valid
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

void AddExceptionDialog::addEntry()
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
    
    if ( text.indexOf(',') != -1)
        params = text.split(',');
    else if ( text.indexOf(';') != -1 )
        params = text.split(',');
    else 
        params << text;
        
    foreach(QString param, params)
    {
        param = param.trimmed();

        //If Exception window
        if ( mode == ADD_MODE | EXCEPTION_MODE )
        {
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
            
            newItems = getWhitelistItems(param, ip);
            
            //it could be a valid item but already on main list
            if ( newItems.isEmpty() )
            {
                errors[param] = "This item is already on the main list";
                valueIsIp[param] = ip;
            }    

        }
        //If Blocklist window
        else if ( mode == ADD_MODE | BLOCKLIST_MODE )
        {
            //FIXME: Needs to be updated to work with the current code
            if ( isValidBlocklist(param) )
                values << param;
            else
                errors[param] = "Not a filepath neither a URL to a valid blocklist";
        }
        
    }
    
    m_notValidTreeWidget->clear();
       
    if ( ! errors.isEmpty() )
    {
         QStringList connections, protocols;
        
        if ( ! groupBox_2->isVisible() )
        {
            groupBox_2->setVisible(true);
            //resize(width(), height() * 2);
        }
        
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
        m_NewItems += newItems;
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




