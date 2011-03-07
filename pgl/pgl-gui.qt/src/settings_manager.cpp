/***************************************** co**********************************
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


#include "settings_manager.h"

SettingsManager::SettingsManager() {


	getUpdateFrequency();
	isLoaded = false;


}


void SettingsManager::addObject( const MoblockSettings &settings ) {


	m_SettingsVector.push_back( settings );
	isLoaded = true;
	initObjects();	

}


void SettingsManager::initObjects() {

	m_Settings = MoblockSettings();
	QVector< QString > baseSettings = m_SettingsVector[0].getSettings();

	for ( int i = 0; i < baseSettings.size(); i++ ) {

		QString setting = baseSettings[i];
		QVector< QString > currentValues = getSettingValues( setting );

		m_Settings.addSetting( setting, currentValues );

	}


}

void SettingsManager::setObject( const MoblockSettings &settings, const int &place ) {
	

	if ( place >= m_SettingsVector.size() ) {
		qDebug() << Q_FUNC_INFO << "Could not replace MoblockSettings object. Place" << place << "is invalid.";
		return;
	}

	m_SettingsVector[place] = settings;

	initObjects();

}


void SettingsManager::setValue( const QString &setting, const QString &value ) {

	m_Settings.setValue( setting, value );

}

void SettingsManager::setValue( const QString &setting, const QVector<QString> &values ) {
	
	m_Settings.setValue( setting, values );

}

void SettingsManager::addValue( const QString &setting, const QString &value ) {

	m_Settings.addValue( setting, value );

}

void SettingsManager::removeValue( const QString &setting, const QString &value ) {

	m_Settings.removeValue( setting, value );

}

QString SettingsManager::getValue( const QString &setting, const int &place ) {

	return m_Settings.getValue( setting, place );

}

QVector< QString > SettingsManager::getValues( const QString &setting ) {

	return m_Settings.getValues( setting );

}

bool SettingsManager::exportToFile( const QString &path ) {


	QVector< QString > baseSettings = m_SettingsVector[0].getSettings();
	MoblockSettings finalSettings = m_SettingsVector[ m_SettingsVector.size() - 1 ];

	for ( int i = 0; i < baseSettings.size(); i++ ) {
		
		QVector< QString > currentValues = m_Settings.getValues( baseSettings[i] );
		QVector< QString > oldValues = getSettingValues( baseSettings[i] );
		
		if ( currentValues != oldValues ) {
			finalSettings.addSetting( baseSettings[i], currentValues );
		}
	}

	return finalSettings.exportToFile( path );

}

QVector< QString > SettingsManager::getSettingValues( const QString &setting ) {

	if ( setting.isNull() ) {
		return QVector<QString>();
	}

	for ( int i = m_SettingsVector.size() - 1; i >= 0; i-- ) {
		QVector< QString > values =  m_SettingsVector[i].getValues( setting );
		if ( !values.isEmpty() ) {
			return values;
		}
	}

	return QVector<QString>();

}

QString SettingsManager::getUpdateFrequency() {

	static QString dailyDir = "/etc/cron.daily/";
	static QString weeklyDir = "/etc/cron.weekly/";
	static QString monthlyDir = "/etc/cron.monthly/";
	
	static QVector< QString > scriptNames;
	if ( scriptNames.size() == 0 ) {
		scriptNames.push_back( "blockcontrol" );
	}

	for ( int i = 0; i < scriptNames.size(); i++ ) {

		m_DailyUpdate = dailyDir + scriptNames[i];
		m_WeeklyUpdate = weeklyDir + scriptNames[i];
		m_MonthlyUpdate = monthlyDir + scriptNames[i];


		if ( QFile::exists( m_DailyUpdate ) ) {
			return m_DailyUpdate;
		}
		else if ( QFile::exists( m_WeeklyUpdate ) ) {
			return m_WeeklyUpdate;
		}
		else if ( QFile::exists( m_MonthlyUpdate ) ) {
			return m_MonthlyUpdate;
		}
	}

	return QString();

}

void SettingsManager::setUpdateFrequency( const QString &frequency ) {
	
	QString oldFreq = getUpdateFrequency();

	if ( oldFreq.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Could not change update frequency. Old frequency is invalid";
		return;
	}
	else if ( frequency.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Could not change update frequency. New frequency is invalid";
		return;
	}
	else if ( oldFreq == frequency ) {
		return;
	}
	//Move the update script to a different cron directory
	m_FreqChange.clear();
	m_FreqChange << "mv" << oldFreq << frequency;

}

void SettingsManager::applyUpdateFrequency() {

	if ( !m_FreqChange.isEmpty() ) {
		m_Root.execute( m_FreqChange );
	}

}

QString SettingsManager::getValueFromBashVar( const QString &setting, const int &place ) {

	QString source; 
	QString result;
	QString variable;
	
	bool inVar = false;
	source = getValue( setting, place );
	if ( source.isEmpty() ) {
		return QString();
	}
	for ( int i = 0; i < source.length(); i++ ) {
		
		if ( inVar == true ) {
			while ( i < source.length() && source[i] != '/' && source[i] != '.' ) {
				variable += source[i];
				i++;
			}
			//Recursive call, as some variables are constructed of other variables
			QString transVar = getValueFromBashVar( variable );
			//In case the variable has not been set in the configuration file, return NULL string
			if ( transVar.isEmpty() ) {
				qWarning() << Q_FUNC_INFO << "Variable" << variable << "was not set, return NULL string.";
				return QString();
			}
			result += transVar;
			inVar = false;
		}
		if ( inVar == false ) {
			//Check if the place in the string is still valid, if not stop the loop
			if ( i >= source.length() ) {
				break;
			}
			//Check if this place is a start of a bash variable
			if ( source[i] == '$' ) {
				inVar = true;
				continue;
			}
			//If tests succed, add the character to the result
			result += source[i];
		}
			
			

	}

	return result;

}

