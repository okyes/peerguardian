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

#include "status_dialog.h"

StatusDialog::StatusDialog( PglCmd *mC, QWidget *parent ) :
	QDialog( parent )
{

	setupUi( this );

	if ( mC == NULL ) {
		m_StatusEdit->setText( tr( "<b>Blockcontrol module is not loaded, information could not be retreived.</b>" ) );
	}
	else {
		m_StatusEdit->setText( tr( "<b>Retrieving detailed information for MoBlock...</b>" ) );
		mC->status();
		connect( mC, SIGNAL( commandOutput( QString ) ), this, SLOT( displayOutput( QString ) ) );
	}


}

void StatusDialog::displayOutput( const QString &output ) {

	QString final;
	QVector<QString> outputV = QVector<QString>::fromList( output.split("\n") );
	if ( outputV.size() <= 2 ) {
		m_StatusEdit->setText( tr( "<b>Could not obtain detailed information for MoBlock.</b>" ) );
	}
	else {
		for ( int i = 0; i < outputV.size(); i++ ) {
				QString fColor = "#404040";
				outputV[i]= outputV[i].trimmed();
			//Useless output from kdesu
			if ( outputV[i].contains( "passprompt", Qt::CaseInsensitive ) ) {
				continue;
			}
			if (  outputV[i].startsWith( "*" ) ) {
				outputV[i] = tr( "<b>%1</b>" ).arg( outputV[i] );
			}
			//Different color here on those special lines
			else if ( outputV[i].startsWith( "Chain", Qt::CaseInsensitive ) ) {
				outputV[i] = tr( "<b>%1</b>" ).arg( outputV[i] );
				if ( outputV[i].contains( "blockcontrol", Qt::CaseInsensitive ) ) {
					fColor = "#3366FF";
				}
				else {
					fColor = "#336600";
				}
			}
			//If the line is empty
			else if ( outputV[i].startsWith( "<br>" ) || outputV[i].isEmpty() ) {
				continue;
			}
			//Set the appropriate color and add a <br> tag to the end of the line	
			outputV[i] = tr( "<font color=\"%1\">%2</font>").arg(fColor).arg(outputV[i]);
			outputV[i].append( "<br>" );
			final += outputV[i];
		}
		int i = final.indexOf( "Current" );
		final.remove( 0, i );
		m_StatusEdit->setText( tr( "<i>%1</i>" ).arg( final ) );

	}

}
