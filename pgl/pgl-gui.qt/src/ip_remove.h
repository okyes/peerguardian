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


#ifndef IP_REMOVE_H
#define IP_REMOVE_H

#include <QWidget>
#include <QList>
#include <QListWidgetItem>
#include <QDialog>
#include <QString>
#include <QVector>
#include <QProcess>

#include "settings_manager.h"
#include "proc_thread.h"

#include "ui_ip_remove.h"

/**
* 
* @short Class representing the "Remove IPs" dialog.
* 
*/

class IpRemove : public QDialog, private Ui::IpRemoveDialog {

	Q_OBJECT
	
	public:
		/**
		 * Constructor. Creates a new "Remove IPs" dialog.
		 * @param sm The SettingsManager object the dialog will get the values from.
		 * @param parent The QWidget parent of this Dialog.
		 */
		IpRemove( SettingsManager *sm, QWidget *parent = 0 );
		/**
		 * Simple function to determine if a setting was changed while the dialog was running.
		 * @return True if a setting was changed, otherwise false.
		 */
		inline bool isSettingChanged() const { return m_SettingChanged; }
	private slots:
		/**
		 * Add the text in the dialog's line edit as a new value in the IP_REMOVE setting in the SettingManager object.
		 */
		void addEntry();
		/**
		 * See which entries are going to be removed from the blocklist if the text in the dialog's line edit is added in the IP_REMOVE setting in the SettingsManager object.
		 */
		void previewEntry();
		/**
		 * Display the preview using the output from the appopriate shell command.
		 * @param text The console output to be displayed.
		 */
		void displayPreview( const QString &output );
		/**
		 * Remove the currently selected value from the IP_REMOVE values vector.
		 */
		void removeEntry();
		/**
		 * Called when a setting is changed. 
		 * After calling this function, isSettingChanged() returns true.
		 */
		void setSettingChanged();
		/**
		 * Update the widgets in the dialog depending on their contents.
		 */
		void updateWidgets();

	private:
		void updateBlocklistDir();
		void updateData();
		void makeConnections();
		bool m_SettingChanged;
		QString m_BlocklistDir;
		SettingsManager *m_Settings;
		QString m_CurrentItem;

		ProcessT proc;

};

#endif //IP_REMOVE_H
