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

#include "moblock_settings.h"
#include "file_transactions.h"

MoblockSettings::MoblockSettings()
{

	formatsInit();

}

MoblockSettings::MoblockSettings( const QString &configPath )
{

	formatsInit();
	setFilePath( configPath );

}
MoblockSettings::~MoblockSettings() {

}

void MoblockSettings::setFilePath( const QString &configPath ) {

	if ( !configPath.isEmpty() ) {
		m_FileName = configPath;
	}
	else {
		qWarning() << Q_FUNC_INFO << "Empty path given, doing nothing";
	}
	//Cleanup
	m_FileLoaded = false;
	m_FileContents = QStringList();
	m_PlacesMap = QMap< QString, int >();
	m_SettingsMap = QMap< QString, QVector<QString> >();
	//Load file
	if ( QFile::exists( configPath ) ) {
		m_FileContents = getFileData( configPath );
		m_FileLoaded = true;
	}
	processSettings();

}

void MoblockSettings::addSetting( const QString &setting, const QVector< QString > &values ) {


	QMap< QString, QString >::const_iterator s = m_Formats.find( setting );
	if ( s == m_Formats.end() ) {
		qWarning() << Q_FUNC_INFO << "Setting" << setting << "has not been defined and was therefore ignored";
		return;
	}

	if ( m_SettingsVector.contains( setting ) ) {
		setValue( setting, values );
		return;
	}

	m_SettingsMap.insert( setting, values );
	m_SettingsVector.push_back( setting );
	m_FileContents.push_back( QString() );
	m_PlacesMap.insert( setting, m_FileContents.size() - 1 );


}



	

void MoblockSettings::processSettings() {
	
	if ( !m_FileLoaded ) {
		qDebug() << Q_FUNC_INFO << "Blockcontrol configuration file is not loaded. Can not process settings";
		return;
	}
	
	for ( int i = 0; i < m_FileContents.size(); i++ ) {
		
		QString line( m_FileContents[i] );
		if ( line.startsWith( "#" ) || line.isEmpty() ) {
			continue;
		}
		else {
			//Remove comments placed after the value, eg INIT="0" #Comment here
			if ( line.contains( "#" ) ) {
				line.remove( line.indexOf( "#" ), line.length() );
				line = line.trimmed();
			}
			//Read raw configuration file data and save them into a map
			QString name;
			QString format;
			QString tmpValues;
			QVector< QString > tmpSetting;
			//Setting name
			name = line.section("=", 0,0 ).trimmed();
			//Setting format from m_Formats map
			QMap< QString, QString >::const_iterator s = m_Formats.find( name );
			if ( s == m_Formats.end() ) {
				qWarning() << Q_FUNC_INFO << "Setting" << name << "has not been defined and was therefore ignored";
				continue;
			}
			else {	
				//Save the setting and its values in the settings map
				format = m_Formats[name];
				tmpValues = line.section( "=", 1, 1 ).trimmed();
				tmpValues = tmpValues.remove( "\"" );
				tmpSetting = processValues( tmpValues, format );
				if ( !tmpSetting.isEmpty() ) {
					m_SettingsMap.insert( name, tmpSetting );
					if ( !m_SettingsVector.contains( name ) ) {
						m_SettingsVector.push_back( name );
					}
					m_PlacesMap.insert( name, i );
				}
				else {
					qDebug() << Q_FUNC_INFO << "Setting string" <<  name << "had no value(s) and as a result, was not inserted into the map";
				}
			}

		}

	}

}

QVector< QString > MoblockSettings::processValues( const QString &setting, const QString &format ) {

	QVector< QString > tmpValues;
	
	if ( format.isNull() || setting.isNull() ) {
		qDebug() << Q_FUNC_INFO << "Unable to process value, function was given emtpy string(s)";
		return tmpValues;
	}
	else {
		//Split depending on the setting's format
		QStringList sList = setting.split( format );
		tmpValues = QVector< QString >::fromList( sList );
	}
	//Remove last vector place if it is empty
	if ( tmpValues.size() > 1 && tmpValues[ tmpValues.size() - 1].isNull() ) {
		tmpValues.remove( tmpValues.size() - 1 );
	}


	return tmpValues;

}

void MoblockSettings::formatsInit() {

	m_Formats.insert( "PATH", SINGLE_STR ); //FIXME: Create a new format(:) for this
	m_Formats.insert( "NAME", SINGLE_STR );
	m_Formats.insert( "DAEMON", SINGLE_STR );
	m_Formats.insert( "DESC", SINGLE_STR );
	m_Formats.insert( "PIDFILE", SINGLE_STR );
	m_Formats.insert( "DAEMON_LOG", SINGLE_STR );
	m_Formats.insert( "STATFILE", SINGLE_STR );
	m_Formats.insert( "CONTROL_NAME", SINGLE_STR );
	m_Formats.insert( "CONTROL_SCRIPT", SINGLE_STR );
	m_Formats.insert( "CONTROL_LOG", SINGLE_STR );
	m_Formats.insert( "CONTROL_LIB", SINGLE_STR );
	m_Formats.insert( "CONTROL_CONF", SINGLE_STR );
	m_Formats.insert( "BLOCKLISTS_LIST", SINGLE_STR );
	m_Formats.insert( "MASTER_BLOCKLIST_DIR", SINGLE_STR );
	m_Formats.insert( "BLOCKLISTS_DIR", SINGLE_STR );
	m_Formats.insert( "ALLOW_IN", SINGLE_STR );
	m_Formats.insert( "ALLOW_OUT", SINGLE_STR );
	m_Formats.insert( "ALLOW_FW", SINGLE_STR );
	m_Formats.insert( "IPTABLES_CUSTOM_INSERT", SINGLE_STR );
	m_Formats.insert( "IPTABLES_CUSTOM_DELETE", SINGLE_STR );
	m_Formats.insert( "MD5SUM_FILE", SINGLE_STR );
	m_Formats.insert( "LSB", SINGLE_STR );
	m_Formats.insert( "STDIFS", SINGLE_STR ); //FIXME: Contains the shell's internal field separator. Probably never used by pgl-gui. Don't know if this would work. (jre)
	m_Formats.insert( "BLOCKLIST_FORMAT", SINGLE_STR );
	m_Formats.insert( "INIT", SINGLE_STR );
	m_Formats.insert( "CRON", SINGLE_STR );
	m_Formats.insert( "WGET_OPTS", SINGLE_STR );
	m_Formats.insert( "TESTHOST", SINGLE_STR );
	m_Formats.insert( "LSB_MODE", SINGLE_STR );
	m_Formats.insert( "IPTABLES_TARGET", SINGLE_STR );
	m_Formats.insert( "NFQUEUE_NUMBER", SINGLE_STR );
	m_Formats.insert( "IPTABLES_SETTINGS", SINGLE_STR );
	m_Formats.insert( "IPTABLES_ACTIVATION", SINGLE_STR );
	m_Formats.insert( "REJECT", SINGLE_STR );
	m_Formats.insert( "REJECT_MARK", SINGLE_STR );
	m_Formats.insert( "REJECT_IN", SINGLE_STR );
	m_Formats.insert( "REJECT_OUT", SINGLE_STR );
	m_Formats.insert( "REJECT_FW", SINGLE_STR );
	m_Formats.insert( "ACCEPT", SINGLE_STR );
	m_Formats.insert( "ACCEPT_MARK", SINGLE_STR );
	m_Formats.insert( "IPTABLES_TARGET_WHITELISTING", SINGLE_STR );
	m_Formats.insert( "WHITE_LOCAL", SINGLE_STR );
	m_Formats.insert( "WHITE_TCP_IN", SPACE_STR );
	m_Formats.insert( "WHITE_UDP_IN", SPACE_STR );
	m_Formats.insert( "WHITE_TCP_OUT", SPACE_STR );
	m_Formats.insert( "WHITE_UDP_OUT", SPACE_STR );
	m_Formats.insert( "WHITE_TCP_FORWARD", SPACE_STR );
	m_Formats.insert( "WHITE_UDP_FORWARD", SPACE_STR );
	m_Formats.insert( "WHITE_IP_IN", SPACE_STR );
	m_Formats.insert( "WHITE_IP_OUT", SPACE_STR );
	m_Formats.insert( "WHITE_IP_FORWARD", SPACE_STR );
	m_Formats.insert( "IP_REMOVE", SEMICOLON_STR );
	m_Formats.insert( "LOG_TIMESTAMP", SINGLE_STR );
	m_Formats.insert( "LOG_SYSLOG", SINGLE_STR );
	m_Formats.insert( "LOG_IPTABLES", SINGLE_STR );
	m_Formats.insert( "VERBOSITY", SINGLE_STR );
	m_Formats.insert( "CRON_MAILTO", SINGLE_STR );
	m_Formats.insert( "E_BADARGS", SINGLE_STR );
	m_Formats.insert( "E_NOTROOT", SINGLE_STR );
	m_Formats.insert( "E_XBIN", SINGLE_STR );
	m_Formats.insert( "E_CONFIG", SINGLE_STR );
	m_Formats.insert( "E_XFILE", SINGLE_STR );
	m_Formats.insert( "E_XCD", SINGLE_STR );
	m_Formats.insert( "E_IPTABLES", SINGLE_STR );
	m_Formats.insert( "E_BLOCKLIST", SINGLE_STR );
	m_Formats.insert( "E_XEXTERNAL", SINGLE_STR );
	m_Formats.insert( "E_NETWORK_DOWN", SINGLE_STR );

}

QString MoblockSettings::getValue( const QString &setting, const int &place ) {
	

	if ( !checkSetting( setting ) ) {
		return QString();
	}
	else if ( place >= m_SettingsMap[setting].size() ) {
		qDebug() << Q_FUNC_INFO << "There is no value at place" << place << "for setting" << setting << ", returning null string";
		return QString();
	}

	return m_SettingsMap[setting][place];

}

QVector< QString > MoblockSettings::getValues( const QString &setting ) {

	if ( !checkSetting( setting ) ) {
		return QVector<QString>();
	}

	return m_SettingsMap[setting];
}

QVector< QString > MoblockSettings::getSettings() {

	return m_SettingsVector;

}

bool MoblockSettings::checkSetting( const QString &setting ) {

	QMap< QString, QVector< QString > >::iterator s = m_SettingsMap.find( setting );
	if ( s == m_SettingsMap.end() ) {
		//qDebug() << "Setting" << setting << "could not be found into the settings vector.";
		return false;
	}
	
	return true;

}

void MoblockSettings::setValue( const QString &setting, const QString &value ) {
	
	if ( !checkSetting(setting ) )
		return;

	m_SettingsMap[setting].replace( 0 , value );


}

void MoblockSettings::setValue( const QString &setting, const QVector< QString > &values ) {

	if ( !checkSetting( setting ) )
		return;
	
	m_SettingsMap[setting] = values;

}

void MoblockSettings::addValue( const QString &setting, const QString &value ) {

	if ( !checkSetting( setting ) )
		return;
	if ( m_SettingsMap[setting].contains( value ) )
		return;
	m_SettingsMap[setting].push_back( value );

}

void MoblockSettings::removeValue( const QString &setting, const QString &value ) {
	
	if ( !checkSetting( setting ) )
		return;
		
	int place = m_SettingsMap[setting].indexOf( value );
	if ( place == -1 ) {
		qDebug() << Q_FUNC_INFO << "Could not remove value" << value << "from" << setting << "as that value does not exist";
		return;
	}
	m_SettingsMap[setting].remove( place );

}

QString MoblockSettings::exportSetting( const QString &setting ) {

	if ( !checkSetting( setting ) )
		 return QString();
	
	QString final;
	QString finalValues;
	QString formatC = m_Formats[setting];
	QVector < QString > values = m_SettingsMap[setting];

	for ( int i = 0; i < values.size(); i++ ) {
		if ( values[i] == formatC || values[i].isEmpty() ) {
			continue;
		}
		finalValues += values[i];
		//If it is single we want no format to be applied
		if ( formatC != SINGLE_STR ) {
			finalValues += formatC;
		}
	}
	//Do not add the format character at the end of the values as it can cause problems in blockcontrol
	if ( finalValues.endsWith( formatC ) ) {
		finalValues.remove( finalValues.lastIndexOf( formatC ), formatC.length() );
	}
	//Create the final line
	final = QObject::tr( "%1=\"%2\"" ).arg( setting ).arg( finalValues );

	return final;

}

bool MoblockSettings::exportToFile( const QString &filename ) {
	
	//Check every place in the vector containing the lines of the file
	//If the setting in that place was changed, replace the line with the new one
	for ( QMap< QString, int >::const_iterator s = m_PlacesMap.begin(); s != m_PlacesMap.end(); s++ ) {
		
		QString setting = s.key();
		int place = s.value();	
		QString fileLine = m_FileContents[place];
		QString newLine = exportSetting( setting );
		if ( fileLine != newLine )
			m_FileContents[place] = newLine;

	}
	
	return saveFileData( m_FileContents, filename );


}
