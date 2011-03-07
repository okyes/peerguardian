/***************************************************************************
 *   Copyright (C) 2007 by Dimitris Palyvos-Giannas   *
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

#include "whois.h"

WhoisDialog::WhoisDialog( const QString &IP,  QWidget *parent ) :
	QDialog( parent )
{
	
	setupUi( this );

	if ( IP.isEmpty() ) {
		m_WhoisBrowser->setText( tr( "Warning, emtpy IP string given, doing nothing" ) );
	}
	else {
		m_Ip = IP;
		m_WhoisBrowser->setHtml(tr( "<b>Retrieving whois information for IP address %2...</b>").arg( m_Ip ) );
		connect( &proc, SIGNAL( commandOutput( QString ) ), this, SLOT( displayWhoisData( QString ) ) );
		proc.execute( "whois", QStringList() << m_Ip );
	}

}


void WhoisDialog::displayWhoisData( const QString &output ) {

	if ( output.isEmpty() ) {
		m_WhoisBrowser->setText( tr( "Failed to get whois data." ) );
		return;
	}

	QVector< QString > rawOutV = QVector<QString>::fromList( output.split( "\n" ) );
	QString final;

	for ( QVector<QString>::iterator s = rawOutV.begin(); s != rawOutV.end(); s++ ) {
		*s = s->trimmed();
		if ( s->startsWith( "%" ) || s->startsWith( "#" ) ) {
			*s = QString();
		}
	}

	final += tr( "<b>Whois information for IP address %2:</b>").arg( m_Ip );

	for ( int i = 0; i < rawOutV.size(); i++ ) {
		if ( rawOutV[i].isEmpty() ) {
			continue;
		}
		final += tr( "<br><font color=\"%1\"><i>* %2</font></i>" ).arg( "#404040" ).arg( rawOutV[i] );
	}
	m_WhoisBrowser->setHtml( final );

}
