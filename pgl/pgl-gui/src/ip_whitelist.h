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

#ifndef IP_WHITELIST_H
#define IP_WHITELIST_H

#include <QDialog>
#include <QWidget>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QProcess>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QList>

#include "settings_manager.h"
#include "proc_thread.h"

#include "whois.h"

#include "ui_ip_whitelist.h"


typedef enum { ELEMENT_IP_COLUMN,
	ELEMENT_TYPE_COLUMN };

typedef enum { TYPE_OUTGOING,
	TYPE_INCOMING,
	TYPE_FORWARD };

/**
*
* @short Class represeting the WhiteList IP dialog
*
*/



class IpWhiteList : public QDialog, private Ui::IpWhiteListDialog {

	Q_OBJECT
	
	public:
		/**
		 * Constructor. Creates a new generic WhiteList dialog.
		 * @param sm The SettingsManager object which the dialog will get the data from.
		 * @param parent The QWidget parent of the dialog.
		 */
		IpWhiteList( SettingsManager *sm, QWidget *parent = 0 );
		/**
		 * Check any setting was changed.
		 * @return True if any setting was changed, otherwise false.
		 */
		inline bool isSettingChanged() const { return m_SettingChanged; }

	private slots:
		/**
		 * Enable/disable the appropriate buttons if an item in the TreeWidget is/is not selected.
		 * Enable/disable the appropriate buttons if the line edit is not/is empty.
		 * Also save the IP of the currently selected item in the m_CurrentItem variable.
		 */
		void updateWidgets();
		/**
		 * Updates the TreeWidget which contains the currently whitelisted IPs with data from the SettingsManager object.
		 */
		void updateData();
		/**
		 * This function calls the isValidIp() function to verify if the text in the line edit is really an IP.
		 * If the test is successful, the function adds the IP address range with the specified type into the whitelist.
		 */
		void addEntry();
		/**
		 * This function uses a separate thread to resolve the hostname. When the execution of the appropriate shell command is over, the writeResolvedEntry() slot is called.
		 */
		void resolveEntry();
		/**
		 * Writes the IPs contained in the given QString in the m_AddEdit text box, separated by semicolons.
		 * @param output The resolve command's output.
		 */
		void displayResolvedEntry( const QString &output );
		/**
		 * Removes the currently selected entry in the dialog's TreeWidget.
		 */
		void removeEntry();
		/**
		 * This function creates a new WhoisDialog for the currently selected IP in the dialog's TreeWidget.
		 */
		void showEntryInfo();
		/**
		 * Called if a setting is changed by the dialog.
		 * After this function is called, isSettingChanged() returns true.
		 */
		void setSettingChanged();
	private:
		void makeConnections();
		bool isValidIp( const QString &text ) const;
		bool areValidIps( const QString &text ) const;
		QString resolveHost( const QString &host );

		bool m_SettingChanged;
		SettingsManager *m_Settings;
		QString m_CurrentItemIp;
		QString m_CurrentItemType;

		ProcessT proc;
};

#endif //IP_WHITELIST_H
