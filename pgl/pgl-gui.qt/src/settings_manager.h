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

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>

#include "pgl_settings.h"

#define BLOCKCONTROL_CONF_PATH "/etc/pgl/pglcmd.conf"

/**
*
* @short Class which loads data from multiple configuration files and saves the changes made into the one which has the highest priority among them.
*
*/


class SettingsManager : public QObject {

	Q_OBJECT

	public:
		/**
		 * Constructor. Creates an empty SettingsManager object.
		 */
		SettingsManager();
		/**
		 * Insert a MoblockSettings object into the objects' vector.
		 * Insert the objects using this function from the one with the smaller priority to the one with the most.
		 * All unsaved changes to the settings are lost when this function is called.
		 * @param settings The MoblockSettings object to be added.
		 */
		void addObject( const MoblockSettings &settings );
		/**
		 * Change the MoblockSetting's object which is saved at the specified place.
		 * All unsaved changes to the settings are lost when this function is called.
		 * @param settings The new MoblockSettings object.
		 * @param place The place of the object. If the place is invalid, this function does nothing.
		 */
		void setObject( const MoblockSettings &settings, const int &place );
		/**
		 * Change the value of the setting given. The setting must have only one value.
		 *  If the setting does not exist, nothing happens.
		 *  If the setting has multiple values, its first value will be changed.
		 * @param setting The setting the value of which you want to change.
		 * @param value The new value of the setting.
		 */
		void setValue( const QString &setting, const QString &value );
		/**
		 * Change the values of the setting given.
		 * If the setting does not exist, nothing happens.
		 * @param setting The setting the value of which you want to change.
		 * @param value The new values of the setting.
		 */
		void setValue( const QString &setting, const QVector< QString > &values );
		/**
		 * Add a value into the setting's vector.
		 * @param setting The setting you want a value to.
		 * @param value The value to be added.
		 */
		void addValue( const QString &setting, const QString &value );
		/**
		 * Removes a value from the setting given.
		 * If the value doesn't exist, nothing happens.
		 * @param setting The setting the value of which you want to remove.
		 * @param value The value which will be removed.
		 */
		void removeValue( const QString &setting, const QString &value );
		/**
		 * Get a value from the setting given.
		 * @param setting The setting you want to get a value from.
		 * @param place The place of the value in the setting's vector.
		 * @return The value of the setting or a null QString if the setting/place is invalid.
		 */
		QString getValue( const QString &setting, const int &place = 0 );
		/** 
		 * Some variables in the blockcontrol defaults configuration file contain bash variables which are set in there.
		 * E.g. CONTROL_LOG="$LOG_DIR/$CONTROL_NAME.log"
		 * This function replaces each bash variable found in the value requested with its real value.
		 * getValueFromBashVar( "CONTROL_LOG" ) would return the string "/var/log/blockcontrol.log" for example,
		 * if LOG_DIR="/var/log" and CONTROL_NAME="blockcontrol".
		 * Note: Since blockcontrol this is no more needed, all variables in blockcontrol.defaults are now defined directly.
		 * @param setting The setting the value of which we want.
		 * @param place The place of the value, in most cases 0.
		 * @return The translated string with the bash variables replaced by their values. If even one of these values was not set in the configuration file, this function returns a NULL string.
		 */
		QString getValueFromBashVar( const QString &setting, const int &place = 0 );
		/**
		 * Get the values vector from the setting given.
		 * @param setting The setting you want to get the value from.
		 * @return A QVector containing the values or an emtpy QVector if the setting does not exist.
		 */
		QVector< QString > getValues( const QString &setting );
		/**
		 * Exports the object with the higher priority in the vector into the specified file.
		 * @param path The path to file where the settings will be exported.
		 * @return True if the file was exported, otherwise false.
		 */
		bool exportToFile( const QString &path );
		/**
		 * Get the frequency of blocklist updates.
		 * This function just checks where the blockcontrol cron script is located.
		 * @return A QString with the update frequency which is the path to the location of the blockcontrol cron script.
		 */
		QString getUpdateFrequency();
		/**
		 * Set the frequency of blocklist updates.
		 * This function doesn't really apply anything. 
		 * applyUpdateFrequency() has to be called if you want the changes to be applied.
		 * @param frequency The path where the blockcontrol cron script will be moved to.
		 */
		void setUpdateFrequency( const QString &frequency );
		/**
		 * Applies the changes to the blocklist update frequency.
		 * This function moves the blockcontrol cron script to the new directory using the SuperUser class.
		 * Does nothing if no change to the frequency has been made.
		 */
		void applyUpdateFrequency();
		/**
		 * @return The location where the daily update script is saved.
		 */
		QString dailyUpdate() const { return m_DailyUpdate; }
		/**
		 * @return The location where the weekly update script is saved.
		 */
		QString weeklyUpdate() const { return m_WeeklyUpdate; }
		/**
		 * @return The location where the monthly update script is saved.
		 */
		QString monthlyUpdate() const { return m_MonthlyUpdate; }


	private:
		void initObjects();
		QVector< QString > getSettingValues( const QString &setting );
		bool isLoaded;
		MoblockSettings m_Settings;
		QMap< QString, bool > m_SettingChanged;
		QVector< MoblockSettings > m_SettingsVector;
		QStringList m_FreqChange;
		SuperUser m_Root;
		QString m_DailyUpdate;
		QString m_WeeklyUpdate;
		QString m_MonthlyUpdate;

	

};


#endif //SETTINGS_MANAGER_H
