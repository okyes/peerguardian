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

#ifndef PGL_SETTINGS_H
#define PGL_SETTINGS_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QVector>
#include <QMap>
#include <QStringList>

#include "super_user.h"

#define PGLCMD_DEFAULTS_PATH "/usr/lib/pgl/pglcmd.defaults"
#define SINGLE_STR "-SIGNLE"
#define SEMICOLON_STR ";"
#define SPACE_STR " "


/**
*
* @short A class used to import, handle and export the data from the blockcontrol defaults configuration file.
*
*/


class MoblockSettings {

	public:
		/**
		 * Constructor. Creates an emtpy MoblockSettings object.
		 */
		MoblockSettings();
		/**
		 * Constructor. Creates a MoblockSettings object and loads the data from the configuration file in configPath.
		 * @param configPath The path to the blockcontrol defaults configuration file.
		 */
		MoblockSettings( const QString &configPath );
		/**
		 * Destructor.
		 */
		~MoblockSettings();
		/**
		 * Add a setting at the end of the file. The setting has to be in the formats multimap. 
		 * If the setting is not recognised this function does nothing.
		 * If the setting already exists, the function calls setValue() to change it.
		 * @param setting The setting to be added.
		 * @param values The setting's values.
		 */
		void addSetting( const QString &setting, const QVector< QString> &values );
		/**
		 * Loads the settings data from the configuration file in the path provided.
		 * If the path is invalid and no path is already set, PGLCMD_DEFAULTS_PATH is used.
		 * @param configPath The path to the configuration file.
		 */
		void setFilePath( const QString &configPath );
		inline QString getFilePath() const { return m_FileName; }
		/**
		 * Change the value of a setting with the new value provided. 
		 * If the setting doesn't exist, this does nothing.
		 * If the setting has more than one values, the first value is changed.
		 * @param setting The setting the value of which you want to change.
		 * @param value The new value to be set.
		 */
		void setValue( const QString &setting, const QString &value );
		/**
		 * Change the values of a setting with the new values provided.
		 * If the setting doesn't exist, this does nothing.
		 * @param setting The setting the values of which you want to change.
		 * @param values The QVector containing the new values
		 */
		void setValue( const QString &setting, const QVector< QString > &values );
		/**
		 * Insert a new value into the setting's vector.
		 * @param setting The setting you want to add the value to.
		 * @param value The value to be added.
		 */
		void addValue( const QString &setting, const QString &value );
		/**
		 * Remove a value from the setting's vector.
		 * Does nothing if the value doesn't exist.
		 * @param setting The setting you want to remove a value from.
		 * @param value The value you want to be removed.
		 */
		void removeValue( const QString &setting, const QString &value ); 
		/**
		 * Get a specific value from the setting's vector.
		 * @param setting The setting you want to get the value from.
		 * @param place The place of the value in the setting's vector.
		 * @return A QString with the value at the specific place or a null QString if this place is invalid.
		 */
		QString getValue( const QString &setting, const int &place = 0 );
		/**
		 * Get the whole setting's vector.
		 * @param setting The setting you want to get the values from.
		 * @return A QVector containing the values of the requested setting or a null QVector if the setting is invalid.
		 */
		QVector< QString > getValues( const QString &setting );
		/**
		 * Return all the settings which were stored in this configuration file.
		 * @return A QVector containing the settings which were loaded from the configuration file.
		 */
		QVector< QString > getSettings();
		/**
		 * Export the setting as a QString to be used in the blockcontrol defaults configuration file.
		 * @param setting The setting to be exported.
		 * @return A QString with the setting as a line of the blockcontrol defaults configuration file or a null string if the setting is invalid.
		 */
		QString exportSetting( const QString &setting );
		/**
		 * Export all the settings contained in this object as a blockcontrol defaults configuration file.
		 * @param filename The path to the file where the settings will be exported.
		 */
		bool exportToFile( const QString &filename );


	private:
		//Split the values contained in a setting string and save them into a vector
		QVector< QString > processValues( const QString &setting, const QString &format );
		//Intiallize the formats for each setting
		void formatsInit();
		//Process all the settings
		void processSettings();
		bool checkSetting( const QString &setting );
		//The lines of the configuration file
		QVector< QString > m_FileContents;
		//The line numbers in the file where each setting is saved
		QMap< QString, int > m_PlacesMap;
		QMap< QString, QString > m_Formats;
		//Stored option values
		QMap< QString, QVector< QString > > m_SettingsMap;
		//All the settings' names are saved in this vector during the intiallization of the object.
		QVector< QString > m_SettingsVector;
		bool m_FileLoaded;
		QString m_FileName;

};

#endif

