/***************************************************************************
 *   Copyright (C) 2007-2008 by Dimitris Palyvos-Giannas   *
 *   jimaras@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ip_whitelist.h"


IpWhiteList::IpWhiteList( SettingsManager *sm, QWidget *parent ) :
	QDialog( parent )
{

	setupUi( this );
	m_SettingChanged = false;

	if ( sm == NULL ) {
		qWarning() << Q_FUNC_INFO << "Null SettingsManager object";
		qWarning() << Q_FUNC_INFO << "The Remove IPs dialog will not work";
		setEnabled( false );
	}
	else {
		m_Settings = sm;
		updateData();
		makeConnections();
		updateWidgets();
	}

}

void IpWhiteList::makeConnections() {

	connect( m_AddButton, SIGNAL( clicked() ), this, SLOT( addEntry() ) );
	connect( m_AddButton, SIGNAL( clicked() ), this, SLOT( setSettingChanged() ) );
	connect( m_ResolveButton, SIGNAL( clicked() ), this, SLOT( resolveEntry() ) );
	connect( m_RemoveButton, SIGNAL( clicked() ), this, SLOT( removeEntry() ) );
	connect( m_RemoveButton, SIGNAL( clicked() ), this, SLOT( setSettingChanged() ) );
	connect( m_WhoisButton, SIGNAL( clicked() ), this, SLOT( showEntryInfo() ) );
	connect( m_WhiteTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( updateWidgets() ) );
	connect( m_AddEdit, SIGNAL( textChanged( QString ) ), this, SLOT( updateWidgets() ) );
	connect( &proc, SIGNAL( commandOutput( QString ) ), this, SLOT( displayResolvedEntry( QString ) ) );

}

void IpWhiteList::updateWidgets() {

	QList< QTreeWidgetItem * >selectedItems = m_WhiteTreeWidget->selectedItems();
	if ( selectedItems.isEmpty() ) {
		m_CurrentItemIp = QString();
		m_CurrentItemType = QString();
		m_RemoveButton->setEnabled( false );
		m_WhoisButton->setEnabled( false );
	}
	else {
		m_CurrentItemIp = selectedItems.first()->text( ELEMENT_IP_COLUMN );
		m_CurrentItemType = selectedItems.first()->text( ELEMENT_TYPE_COLUMN );
		m_RemoveButton->setEnabled( true );
		m_WhoisButton->setEnabled( true );
	}
	m_AddButton->setEnabled( !m_AddEdit->text().isEmpty() );
	m_ResolveButton->setEnabled( !m_AddEdit->text().isEmpty() && !areValidIps( m_AddEdit->text().trimmed() ) );
		
	

}

void IpWhiteList::updateData() {

	QVector< QString > whiteOUT = m_Settings->getValues( "WHITE_IP_OUT" );
	QVector< QString > whiteIN = m_Settings->getValues( "WHITE_IP_IN" );
	QVector< QString > whiteFWD = m_Settings->getValues( "WHITE_IP_FORWARD" );


	m_WhiteTreeWidget->clear();

	for ( int i = 0; i < whiteOUT.size(); i++) {
		whiteOUT[i] = whiteOUT[i].trimmed();

		if ( !whiteOUT[i].isEmpty() ) {
			new QTreeWidgetItem( m_WhiteTreeWidget, QStringList() << whiteOUT[i] << tr( "Outgoing" ) );
		}
		
	}
	
	for ( int i = 0; i < whiteIN.size(); i++ ) {
		whiteIN[i] = whiteIN[i].trimmed();

		if ( !whiteIN[i].isEmpty() ) {
			new QTreeWidgetItem( m_WhiteTreeWidget, QStringList() << whiteIN[i] << tr( "Incoming" ) );
		}
			
	}

	for ( int i = 0; i < whiteFWD.size(); i++ ) {
		whiteFWD[i] = whiteFWD[i].trimmed();

		if ( !whiteFWD[i].isEmpty() ) {
			new QTreeWidgetItem( m_WhiteTreeWidget, QStringList() << whiteFWD[i] << tr( "Forward" ) );
		}
		
	}

	m_WhiteTreeWidget->resizeColumnToContents( ELEMENT_IP_COLUMN );

}

void IpWhiteList::addEntry() {
	

	QString text = m_AddEdit->text().trimmed();
	bool added = false;
	
	QVector< QString > parts = QVector<QString>::fromList( text.split( ";", QString::SkipEmptyParts ) );

	for ( int i = 0; i < parts.size(); i++ ) {

		if ( !isValidIp( parts[i] ) ) {
			int choise = QMessageBox::warning( this, tr( "Possibly invalid IP" ), tr( "Entry \"%1\" does not appear to be a valid IP address/range.\nIf this entry is a hostname you are strongly urged to resolve it first using the \"Resolve\" button.\n\nWhitelist the above entry anyway?\n").arg( parts[i] ), QMessageBox::Yes,QMessageBox::No );
			if ( choise == QMessageBox::No ) {
				continue;
			}
		}
		
		switch( m_AddTypeCombo->currentIndex() ) {
			case TYPE_OUTGOING:
				m_Settings->addValue( "WHITE_IP_OUT", parts[i] );
				added = true;
				break;	

			case TYPE_INCOMING:
				m_Settings->addValue( "WHITE_IP_IN", parts[i] );
				added = true;
				break;
	
			case TYPE_FORWARD:
				m_Settings->addValue( "WHITE_IP_FORWARD", parts[i] );
				added =true;
				break;
	
			default:
				qWarning() << Q_FUNC_INFO << "Invalid connection type, doing nothing";
			}

	}


	if ( added == true ) {
		m_AddEdit->clear();
		//FIXME: See if it is better this way
		m_AddTypeCombo->setCurrentIndex( 0 );
		updateData();
	}

}


void IpWhiteList::resolveEntry() {


	QString host = m_AddEdit->text().trimmed();

	if ( host.isEmpty() ) {
		return;
	}
	else if ( isValidIp( host ) ) {
		QMessageBox::warning( this, tr( "Could not resolve hostname" ), tr( "Entry %1 seems to be a valid IP address and as a result was not resolved.").arg(host), QMessageBox::Ok  );
		return;
	}

	proc.execute( "dig", QStringList() << "+short" << host );


}


void IpWhiteList::displayResolvedEntry( const QString &output ) {

	QString result;

	//TODO: Improve the error message
	if ( output.isEmpty() ) {
		QMessageBox::warning( this, tr( "Could not resolve hostname" ), tr( "Failed to resolve hostname."), QMessageBox::Ok );
		return;
	}

	QVector<QString> testRes = QVector<QString>::fromList( output.split( "\n" ) );
	for ( int i = 0; i < testRes.size(); i++ ) {
		if ( isValidIp( testRes[i] ) ) {
			result += testRes[i];
			result += ";";
		}
	}

		
	if ( result.isEmpty() ) {
		//TODO: "Smart" error messages using data from the console output.
		QMessageBox::warning( this, tr( "Could not resolve hostname" ), tr( "Failed to resolve hostname."), QMessageBox::Ok );
		return;
	}	

	m_AddEdit->setText( result );

}


void IpWhiteList::removeEntry() {

	if ( m_CurrentItemType.isEmpty() ) {
		return;
	}
	
	if ( m_CurrentItemType == "Outgoing" ) {
		m_Settings->removeValue( "WHITE_IP_OUT", m_CurrentItemIp );
	}
	else if ( m_CurrentItemType == "Incoming" ) {
		m_Settings->removeValue( "WHITE_IP_IN", m_CurrentItemIp );
	}
	else if ( m_CurrentItemType == "Forward" ) {
		m_Settings->removeValue( "WHITE_IP_FORWARD", m_CurrentItemIp );
	}
	else {
		qWarning() << Q_FUNC_INFO << "Invalid item type, doing nothing";
	}
	
	//TODO: Return to the old row if possible.
	updateData();	

}

void IpWhiteList::showEntryInfo() {

	if ( m_CurrentItemIp.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Emtpy item IP, doing nothing";
		return;
	}

	WhoisDialog *whois = new WhoisDialog( m_CurrentItemIp, this );
	whois->exec();
	delete whois;

}



void IpWhiteList::setSettingChanged() {

	m_SettingChanged = true;

}


bool IpWhiteList::isValidIp( const QString &text ) const {

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

bool IpWhiteList::areValidIps( const QString &text ) const {


	QVector< QString > parts = QVector<QString>::fromList( text.split( ";", QString::SkipEmptyParts  ));

	for ( int i = 0; i < parts.size(); i++ ) {

		if ( !isValidIp( parts[i] ) ) {
			return false;
		}
	}

	return true;

	

}
