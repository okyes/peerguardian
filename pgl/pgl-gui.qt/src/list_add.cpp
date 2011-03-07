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

#include "list_add.h"

ListAddDialog::ListAddDialog( QWidget *parent ) :
	QDialog( parent ) 
{

	setupUi( this );
	option = NONE;

	g_MakeConnections();

}

void ListAddDialog::g_MakeConnections() {

	connect( m_NoTimeStampBox, SIGNAL( clicked( bool ) ), this, SLOT( g_ToggleListTimeStamp( bool ) ) );
	connect( m_LocalBox, SIGNAL( clicked( bool ) ), this, SLOT( g_ToggleListLocal( bool ) ) );
	connect( m_FileSelectButton, SIGNAL( clicked() ), this, SLOT( g_BrowseListFile() ) );

}



void ListAddDialog::g_ToggleListLocal( bool enabled ) {

	if ( enabled == true ) {
		m_NoTimeStampBox->setChecked( false );
		option = LOCAL;
	}
	else {
		if ( option == LOCAL ) {
			option = NONE;
		}
	}

}

void ListAddDialog::g_ToggleListTimeStamp( bool enabled ) {

	if ( enabled == true ) {
		m_LocalBox->setChecked( false );
		option = NO_TIME_STAMP;
	}
	else {
		if ( option == NO_TIME_STAMP ) {
			option = NONE;
		}
	}

}

void ListAddDialog::g_BrowseListFile() {

	QString path = QFileDialog::getOpenFileName( this, tr( "Choose the appropriate file" ), "/home/", tr( "Blocklists( *.gz *.7z *.dat *.p2b *.p2p )" ) );

	if ( path.isNull() ) {
		return;
	}
	m_LocalBox->setChecked( true );
	g_ToggleListLocal( true );
	m_ListLocationEdit->setText( path );

}

bool ListAddDialog::g_IsValid() const {

	//Send is as a disabled item so that its extension is checked
	QString text = "#" + g_GetLocation();
	if ( text.isNull() ) {
		return false;
	}
	ListItem testItem( text );
	if ( testItem.mode == COMMENT_ITEM ) {
		return false;
	}

	return true;

}
