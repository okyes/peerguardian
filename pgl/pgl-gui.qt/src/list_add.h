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

#ifndef LIST_ADD_H
#define LIST_ADD_H

#include <QString>
#include <QFileDialog>

#include "ui_list_add.h"
#include "pgl_lists.h"

/**
*
* @short Simple class to allow the additions of new blocklist entries
*
*/

class ListAddDialog : public QDialog, private Ui::ListAddDialog {

	Q_OBJECT

	public:
		/**
		 * Constructor. Setups the user interface and intiallizes the variables.
		 * @param parent
		 */
		ListAddDialog( QWidget *parent = 0 );
		/**
		 * Create the connections between the objects.
		 */
		void g_MakeConnections();
		/**
		 * Get the new blocklist's location.
		 * @return QString containing the blocklist's location.
		 */
		QString g_GetLocation() const { return m_ListLocationEdit->text(); }
		/**
		 * Check if the text in the location edit is a valid list entry.
		 * @return True if it is valid, otherwise false.
		 */
		bool g_IsValid() const;
		listOption option;
	
	private slots:
		void g_ToggleListLocal( bool enabled );
		void g_ToggleListTimeStamp( bool enabled );
		void g_BrowseListFile();

		
		

};


#endif
