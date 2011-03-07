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

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QFileDialog>
#include <QtDebug>


#include "ui_settings.h"
#include "pglcmd.h"
#include "peerguardian_info.h"
#include "pgl_settings.h"
#include "pgl_lists.h"
#include "peerguardian_log.h"
#include "settings_manager.h"

class QString;

/**
 *
 * @short Class representing the mobloquer settings dialog.
 *
 */


class SettingsDialog : public QDialog, private Ui::SettingsDialog {

	Q_OBJECT

	public:
		/**
		 * Constructor. 
		 * Intiallizes the UI.
		 * @param parent The QWidget parent of the this object.
		 */
		SettingsDialog( QWidget *parent = 0 );
		/**
		 * Intiallize the custom connections between objects.
		 */
		void makeConnections();
		/**
		 * Set the text in the "log" line edit;
		 * @param path QString with the new text.
		 */
		inline void file_SetLogPath( const QString &path ) { 	m_LogPathEdit->setText( path ); }
		/**
		 * Set the text in the "blocklists" line edit.
		 * @param path QString with the new text.
		 */
		inline void file_SetListsPath( const QString &path ) { m_ListsPathEdit->setText( path ); }
		/**
		 * Set the text in the "blockcontrol defaults configuration file" line edit.
		 * @param path QString with the new text.
		 */
		inline void file_SetDefaultsPath( const QString &path ) { m_DefaultsPathEdit->setText( path ); }
		/**
		 * Set the text in the "blockcontrol configuration file" line edit.
		 * @param path QString with the new text.
		 */
		inline void file_SetConfPath( const QString &path ) { m_ConfPathEdit->setText( path ); }
		/**
		 * Set the text in the "blockcontrol" line edit.
		 * @param path QString with the new text.
		 */
		inline void file_SetControlPath( const QString &path ) { m_ControlPathEdit->setText( path ); }
		/**
		 * Set the text in the "blockcontrol log" line edit.
		 * @param path QString with the new text.
		 */
		inline void file_SetControlLogPath( const QString &path ) { m_ControlLogPathEdit->setText( path ); }
		/**
		 * Set the text in the "Sudo frontend" line edit.
		 * @param path QString with the new text.
		 */
		inline void file_SetRootPath( const QString &path ) { m_RootPathEdit->setText( path ); }
		/**
		 * Get the "log" line edit's text.
		 * @return QString with the text of the line edit.
		 */
		inline QString file_GetLogPath() const { return m_LogPathEdit->text(); }
		/**
		 * Get the "blocklists" line edit's text.
		 * @return QString with the text of the line edit.
		 */
		inline QString file_GetListsPath() const { return m_ListsPathEdit->text(); }
		/**
		 * Get the "blockcontrol defaults configuration file" line edit's text.
		 * @return QString with the text of the line edit.
		 */
		inline QString file_GetDefaultsPath() const { return m_DefaultsPathEdit->text(); }
		/**
		 * Get the "blockcontrol configuration file" line edit's text.
		 * @return QStrign with the text of the line edit.
		 */
		inline QString file_GetConfPath() const { return m_ConfPathEdit->text(); }
		/**
		 * Get the "blockcontrol" line edit's text.
		 * @return QString with the text of the line edit.
		 */
		inline QString file_GetControlPath() const { return m_ControlPathEdit->text(); }
		/**
		 * Get the "blockcontrol log" line edit's text.
		 * @return QString with the text of the line edit.
		 */
		inline QString file_GetControlLogPath() const { return m_ControlLogPathEdit->text(); }
		/**
		 * Get the "sudo front end" line edit's text.
		 * @return QString with the text of the line edit.
		 */
		inline QString file_GetRootPath() const { return m_RootPathEdit->text(); }

	private slots:
		/**
		 * Show the QFileDialog so the user can select a new log file.
		 */
		void file_BrowseLogPath();
		/**
		 * Show the QFileDialog so the user can select a new blocklists file.
		 */
		void file_BrowseListsPath();
		/**
		 * Show the QFileDialog so the user can select a new blockcontrol defaults configuration file.
		 */
		void file_BrowseDefaultsPath();
		/**
		 * Show the QFileDialog so the user can select a new default blockcontrol defaults configuration file.
		 */
		void file_BrowseConfPath();
		/**
		 * Show the QFileDialog so the user can select a new blockcontrol script file.
		 */
		void file_BrowseControlPath();
		/**
		 * Show the QFileDialog so the user can select a new blockcontrol log file.
		 */
		void file_BrowseControlLogPath();
		void file_BrowseRootPath();
		void file_SetDefaults();
		
};

#endif

