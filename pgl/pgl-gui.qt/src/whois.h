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

#ifndef WHOIS_DIALOG_H
#define WHOIS_DIALOG_H

#include <QProcess>
#include <QVector>
#include <QtDebug>

#include "proc_thread.h"

#include "ui_whois.h"

class QString;
class QStringList;

/**
 *
 * @short Simple class representing the window which provides whois information.
 *
 */

class WhoisDialog : public QDialog, private Ui::WhoisDialog {

	Q_OBJECT

	public:
		/**
		 * Constructor. Intiallzes the GUI and then searches for whois information for the IP given.
		 * @param IP The IP you want to get whois information for.
		 * @param parent The QWidget parent of this object.
		 */
		WhoisDialog( const QString &IP, QWidget *parent = 0 );

	private slots:
		/**
		 * Show the whois data after the process is finished.
		 * @param exitCode The exit code of the process.
		 */
		void displayWhoisData( const QString &output );

	private:
		ProcessT proc;
		QString m_Ip;



};

#endif
