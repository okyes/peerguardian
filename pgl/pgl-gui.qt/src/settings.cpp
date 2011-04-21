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

#include "settings.h"
#include "peerguardian_log.h"

SettingsDialog::SettingsDialog( QWidget *parent ) :
	QDialog( parent )
{


	setupUi( this );

	makeConnections();
}

void SettingsDialog::makeConnections() {

	connect( m_LogPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseLogPath() ) );
	connect( m_ListsPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseListsPath() ) );
	connect( m_DefaultsPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseDefaultsPath() ) );
	connect( m_ConfPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseConfPath() ) );
	connect( m_ControlPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseControlPath() ) );
	connect( m_ControlLogPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseControlLogPath() ) );
	connect( m_RootPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseRootPath() ) );
	connect( m_SetDefaultsButton, SIGNAL( clicked() ), this, SLOT( file_SetDefaults() ) );
}

void SettingsDialog::file_BrowseLogPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/var/log/" );
	if ( path.isNull() )
		return;
	m_LogPathEdit->setText( path );

}

void SettingsDialog::file_BrowseControlLogPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/var/log/" );
	if ( path.isNull() )
		return;

	m_ControlLogPathEdit->setText( path );

}
void SettingsDialog::file_BrowseListsPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/etc/blockcontrol/" );
	if ( path.isNull() )
		return;

	m_ListsPathEdit->setText( path );

}

void SettingsDialog::file_BrowseDefaultsPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/etc/blockcontrol/" );
	if ( path.isNull() )
		return;

	m_DefaultsPathEdit->setText( path );

}

void SettingsDialog::file_BrowseConfPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/etc/blockcontrol/" );
	if ( path.isNull() )
		return;

	m_ConfPathEdit->setText( path );

}


void SettingsDialog::file_BrowseControlPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/usr/bin/" );
	if ( path.isNull() )
		return;

	m_ControlPathEdit->setText( path );

}

void SettingsDialog::file_BrowseRootPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/usr/bin" );

	if ( path.isNull() )
		return;

	m_RootPathEdit->setText( path );

}
	

void SettingsDialog::file_SetDefaults() {

	file_SetLogPath( PeerguardianLog::getFilePath() );
	file_SetListsPath( PGL_LIST_PATH );
	file_SetDefaultsPath( PGLCMD_DEFAULTS_PATH );
	file_SetConfPath( PGLCMD_CONF_PATH );
	file_SetControlPath( PGLCMD_PATH );
	file_SetControlLogPath( PeerguardianLog::getFilePath() );
	file_SetRootPath( KDESU_PATH );
	
}
