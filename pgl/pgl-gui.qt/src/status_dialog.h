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

#ifndef STATUS_DIALOG_H
#define STATUS_DIALOG_H

#include <QObject>
#include <QDialog>
#include <QString>
#include <QVector>
#include <QtDebug>

#include "pglcmd.h"
#include "ui_status_dialog.h"

/**
* 
* 
* @short Simple class representing the dialog which shows the output of blockcontrol status.
*
*/

class StatusDialog : public QDialog, private Ui::StatusDialog {

	Q_OBJECT

	public:
		/**
		 * Constructor. Create a new StatusDialog and get moblock's status.
		 * @param mC A pointer to a PglCmd object which will give the data.
		 * @param parent The QWidget parent of this dialog.
		 */
		StatusDialog( PglCmd *mC, QWidget *parent = 0 );
		~StatusDialog() { }

	public slots:
		/**
		 * Display the output of the status command.
		 * @param output The output of the status command.
		 */
		void displayOutput( const QString &output );



};


#endif //STATUS_DIALOG_H
