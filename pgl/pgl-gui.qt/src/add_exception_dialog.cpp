
#include "add_exception_dialog.h" 

AddExceptionDialog::AddExceptionDialog(QWidget *parent, int mode) :
	QDialog( parent )
{
	setupUi( this );
    
    ports["HTTP"] = 80;
    ports["HTTPS"] = 443;
    ports["FTP"] = 21;
    ports["POP"] = 110;
    ports["SMTP"] = 587;
    ports["IMAP"] = 143;

    if ( mode == (ADD_MODE | EXCEPTION_MODE) )
    {       
        
        QString help = "Valid Inputs: You can enter an IP Address with";
        help +=  " or without mask and a port number or name (eg. http, ftp, etc).\n";
        help += "You can enter multiple items separated by ';' or ',' .";
                    
            m_helpLabel->setText(QObject::tr(help.toUtf8 ()));
            m_browseButton->hide();
            m_statusCombo->setCurrentIndex (1);
            m_statusCombo->setEnabled(false);
    }
    else if ( mode == (ADD_MODE | BLOCKLIST_MODE) )
    {
            m_helpLabel->setText(QObject::tr("Valid Inputs: You can enter a local path or an address to a valid blocklist."));
            m_addTypeCombo->setEnabled(false);
            m_statusCombo->setEnabled(false);
    }
    /*else if ( mode == (EDIT_MODE | EXCEPTION_MODE) )
    {
            m_exceptionTreeWidget->hide();
            m_addButton->hide();
            m_helpLabel->hide();
            resize(width(), 300);
    }
    else if ( mode == (EDIT_MODE | BLOCKLIST_MODE))
    {
            m_addTypeCombo->setEnabled(false);
            m_statusCombo->setEnabled(false);
    }*/
    
    connect(m_addEdit, SIGNAL(returnPressed()), this, SLOT(addEntry()));
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
}

AddExceptionDialog::~AddExceptionDialog()
{
}


void AddExceptionDialog::addEntry()
{
    QString text = m_addEdit->text();
    QString desc = "";
    QStringList params, info;
    bool valid = false;
    int port;
    
    if ( text.indexOf(',') != -1)
        params = text.split(',');
    
    if ( text.indexOf(';') != -1 )
        params = text.split(',');
    
    if ( params.isEmpty() )
        params << text;
        
    foreach(QString param, params)
    {
        param = param.trimmed();
        valid = false;

        port = getPort(param);
        if ( port >= 0 )
        {
            if ( QString::number(port) != param )
                desc = "Port " + QString::number(port);
            else
                desc = "Port";
                //param += "(" + QString::number(port) + ")";
            valid = true;
        }
        else if ( isValidIp(param) ){
            desc = "IP Address";
            valid = true;
        }
        
        if ( valid )
        {
            info << param;
            info << m_statusCombo->currentText();
            info << m_addTypeCombo->currentText();
            info << desc;
            QTreeWidgetItem * item = new QTreeWidgetItem(m_exceptionTreeWidget, info);
            m_exceptionTreeWidget->addTopLevelItem(item);
        }
        
        info.clear();
    }
    
    m_addEdit->clear();
    
}


int AddExceptionDialog::getPort(const QString& p)
{
    if ( ports.contains(p.toUpper()) )
        return ports[p.toUpper()];
    
    if ( p == QString::number(p.toInt()) )
        return p.toInt();
    
    return -1;
    
}

bool AddExceptionDialog::isValidIp( const QString &text ) const {

	QString ip = text.trimmed();

	if ( ip.isEmpty() ) {
		return false;
	}
	else {
		//Split the string into two sections
		//For example the string 127.0.0.1/24 will be split into two strings:
		//mainIP = "127.0.0.1"
		//range = "24"
		QVector< QString > ipSections = QVector< QString >::fromList( ip.split( "/" ) );
		if ( ipSections.size() < 1 || ipSections.size() > 2 ) {
			return false;
		}
		QString mainIp = ipSections[0];
		QString range = ( ipSections.size() == 2 ) ? ipSections[1] : QString();
		//Split the IP address 
		//E.g. split 127.0.0.1 to "127", "0", "0", "1"
		QVector< QString > ipParts = QVector<QString>::fromList( mainIp.split( "." ) );
		//If size != 4 then it's not an IP
		if ( ipParts.size() != 4 ) {
			return false;
		}
		
		for ( int i = 0; i < ipParts.size(); i++ ) {
			if ( ipParts[i].isEmpty() ) {
				return false;
			}
			//Check that every part of the IP is a positive  integers less or equal to 255
			if ( QVariant( ipParts[i] ).toInt() > 255 || QVariant( ipParts[i] ).toInt() < 0 ) {
				return false;
			}
			for ( int j = 0; j < ipParts[i].length(); j++ ) {
				if ( !ipParts[i][j].isNumber() ) {
					return false;
				}
			}
		}
		//Check if the range is a valid subnet mask
		if ( !isValidIp( range ) ) {
			//Check that the range is a positive integer less or equal to 24
			if ( QVariant( range ).toInt() <= 24 && QVariant( range ).toInt() >= 0 ) {
				for ( int i = 0; i < range.length(); i++ ) {
					if ( !range[i].isNumber() ) {
						return false; 
					}
			}
		}
			else {
				return false;
			}
		}
	}
		

	return true;

}

void AddExceptionDialog::keyPressEvent ( QKeyEvent * e )
{   
    // have tried many different things in here
    qDebug( "Dialog key pressed %d", e->key() );
    
    if (e->key() == Qt::Key_Escape)
        QDialog::keyPressEvent (e);
        
    //else
    //    e->ignore();  
}




