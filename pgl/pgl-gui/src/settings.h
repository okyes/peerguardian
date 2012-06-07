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
#include <QSettings>


#include "ui_settings.h"
#include "pglcmd.h"
#include "peerguardian_info.h"
#include "pgl_settings.h"
#include "pgl_lists.h"

class QString;

/**
 *
 * @short Class representing the pglgui settings dialog.
 *
 */


class SettingsDialog : public QDialog, private Ui::SettingsDialog {

	Q_OBJECT
    
    QSettings * m_Settings;

	public:
		/**
		 * Constructor.
		 * Intiallizes the UI.
		 * @param parent The QWidget parent of the this object.
		 */
		SettingsDialog( QSettings  *settings , QWidget *parent = 0);
		/**
		 * Set the text in the "Sudo frontend" line edit.
		 * @param path QString with the new text.
		 */
		inline void file_SetRootPath( const QString &path ) { m_RootPathEdit->setText( path ); }

		 /* Get the "sudo front end" line edit's text.
		 * @return QString with the text of the line edit.
		 */
		inline QString file_GetRootPath() const { return m_RootPathEdit->text(); }
        
        inline int getMaxLogEntries() const { return m_MaxLogEntries->value(); }

	private slots:
		void file_BrowseRootPath();
		void file_SetDefaults();
		
};

#endif

