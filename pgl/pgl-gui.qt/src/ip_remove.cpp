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

#include "ip_remove.h"

IpRemove::IpRemove( SettingsManager *sm, QWidget *parent ) :
	QDialog( parent )
{
	setupUi( this );

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

void IpRemove::makeConnections() {

	connect( m_AddButton, SIGNAL( clicked() ), this, SLOT( setSettingChanged() ) );
	connect( m_AddButton, SIGNAL( clicked() ), this, SLOT( addEntry() ) );
	connect( m_PreviewButton, SIGNAL( clicked() ), this, SLOT( previewEntry() ) );
	connect( m_RemoveButton, SIGNAL( clicked() ), this, SLOT( removeEntry() ) );
	connect( m_RemoveButton, SIGNAL( clicked() ), this, SLOT( setSettingChanged() ) );
	connect( m_RemovedListWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( updateWidgets() ) );
	connect( m_AddEdit, SIGNAL( textChanged( QString ) ), this, SLOT( updateWidgets() ) );
	connect( &proc, SIGNAL( commandOutput( QString ) ), this, SLOT( displayPreview( QString ) ) );


}

void IpRemove::updateData() {

	if ( m_Settings == NULL ) { 
		qWarning() << Q_FUNC_INFO << "Null SettingsManager object";
		qWarning() << Q_FUNC_INFO << "The Remove IPs dialog will not work";
		return;
	}

	updateBlocklistDir();

	m_RemovedListWidget->clear();
	QVector< QString > removedIPs = m_Settings->getValues( "IP_REMOVE" );
	for ( int i = 0; i < removedIPs.size();i++ ) {

		if ( removedIPs[i].isEmpty() ) {
			continue;
		}
		m_RemovedListWidget->addItem( removedIPs[i] );

	}
	

}

void IpRemove::updateBlocklistDir() {

	//Get the blocklist dir
	QString listDir = m_Settings->getValueFromBashVar( "MASTER_BLOCKLIST_DIR" );
	if ( listDir.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Unable to determine the location of the blocklists. Check the settings in your blockcontrol configuration file";
		m_BlocklistDir = QString();
		return;
	}
	QString bType = m_Settings->getValue( "BLOCKLIST_FORMAT");
	QString bName;

	if ( bType == "d" ) {
		bName = "ipfilter.dat";
	}
	else if ( bType == "p" ) {
		bName = "guarding.p2p";
	}
	else if ( bType == "n" ) { 
		bName = "guarding.p2b";
	}
	else {
		qWarning() << Q_FUNC_INFO << "Not enough information to get blocklist file name";
		m_BlocklistDir = QString();
		return;
	}
	m_BlocklistDir = tr( "%1/%2").arg(listDir).arg(bName);
	
}


void IpRemove::updateWidgets() {

	QList< QListWidgetItem * >selectedItems = m_RemovedListWidget->selectedItems();
	if ( selectedItems.isEmpty() ) {
		m_CurrentItem = QString();
		m_RemoveButton->setEnabled( false );
	}
	else {
		m_CurrentItem = selectedItems.first()->text();
		m_RemoveButton->setEnabled( true );
	}
	m_AddButton->setEnabled( !m_AddEdit->text().isEmpty() );
	m_PreviewButton->setEnabled( !m_AddEdit->text().isEmpty() );
		

}

void IpRemove::addEntry() {

	if ( m_AddEdit->text().isEmpty() ) {
		return;
	}
	m_Settings->addValue( "IP_REMOVE", m_AddEdit->text().trimmed() );
	
	m_AddEdit->clear();
	updateData();


}

void IpRemove::previewEntry() {

	QString text  = m_AddEdit->text().trimmed();
	
	if ( text.isEmpty()  ) {
		qDebug() << Q_FUNC_INFO << "Empty text given. Could not display preview.";
		return;
	}
	if ( m_BlocklistDir.isEmpty() || !QFile::exists( m_BlocklistDir ) ) {
		qDebug() << Q_FUNC_INFO << "Invalid blocklist path given. Could not display preview.";
		return;
	}

	proc.execute( "grep", QStringList() << "-i" << text << m_BlocklistDir );

}

void IpRemove::removeEntry() {

	if ( m_CurrentItem.isEmpty() ) {
		return;
	}
	m_Settings->removeValue( "IP_REMOVE", m_CurrentItem );

	updateData();

}

void IpRemove::setSettingChanged() {

	m_SettingChanged = true;

}

void IpRemove::displayPreview( const QString &output ) {


	QVector< QString > outputLines = QVector<QString>::fromList( output.split( "\n" ) );
	m_PreviewListWidget->clear();
	for ( int i = 0; i < outputLines.size(); i++ ) {
		
		if ( outputLines[i].isEmpty() ) {
			continue;
		}

		m_PreviewListWidget->addItem( outputLines[i].trimmed() );

	}

}

