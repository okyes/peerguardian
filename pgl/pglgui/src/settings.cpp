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
#include "super_user.h"

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent) :
	QDialog( parent )
{
	setupUi( this );
    
    
    file_SetRootPath( SuperUser::sudoCommand() );
    int val = settings->value("maximum_log_entries").toInt();
    m_MaxLogEntries->setValue(val);
    
    connect( m_RootPathButton, SIGNAL( clicked() ), this, SLOT( file_BrowseRootPath() ) );

}


void SettingsDialog::file_BrowseRootPath() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/usr/bin" );

	if ( path.isNull() )
		return;

	m_RootPathEdit->setText( path );

}
	

void SettingsDialog::file_SetDefaults() {

	file_SetRootPath( SuperUser::sudoCommand() );
}
