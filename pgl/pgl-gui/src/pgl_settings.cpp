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

#include "pgl_settings.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QObject>

#include "file_transactions.h"
#include "utils.h"



QHash<QString, QString> PglSettings::variables = QHash<QString, QString>();
QString mPglcmdDefaultsPath = "";
QString mLastError = "";


QString PglSettings::getVariableInValue(const QString & var)
{
    QString variable(var);

    if ( var.contains("$") )
    {
        QString strippedVar(var);
        int n = 0;

        if ( var.contains("{") && var.contains("}") )
        {
            int pos1 = var.indexOf('{') + 1;
            n = var.indexOf('}') - pos1;
            strippedVar = var.mid(pos1, n);
            n = 3;
        }
        else
        {
            int pos1 = var.indexOf('$') + 1;
            strippedVar = var.mid(pos1);
            n = 1;
        }

        if ( variables.contains(strippedVar) )
            return variable.replace(var.indexOf("$"), strippedVar.size()+n, variables[strippedVar]);
    }


    return var;
}

QString PglSettings::getValueInLine(const QString& line)
{

    QString value = getValue(line);

    if ( ! value.contains("$") )
        return value;

    QString newValue("");

    if ( value.contains("/") )
    {

        foreach(QString val, value.split("/", QString::SkipEmptyParts))
        {
            newValue += getVariableInValue(val);
            newValue += "/";
        }
    }
    else
        newValue += getVariableInValue(value);

    if ( newValue.endsWith("/") )
        newValue.remove(newValue.size()-1, 1);

    return newValue;
}

QString PglSettings::findPglcmdDefaultsPath()
{
    if (QFile::exists(PGLCMD_DEFAULTS_PATH1)) 
        return PGLCMD_DEFAULTS_PATH1;
    
    if (QFile::exists(PGLCMD_DEFAULTS_PATH2)) 
        return PGLCMD_DEFAULTS_PATH2;
    
    QDir currentDir = QDir::current();
    currentDir.cdUp();
    
    if (! currentDir.cd("lib") )
        return "";
    
    if (! currentDir.exists("pgl"))
        return "";
    
    currentDir.cd("pgl");
    return currentDir.absolutePath();
}

bool PglSettings::loadSettings()
{
    mPglcmdDefaultsPath = findPglcmdDefaultsPath();
    if (mPglcmdDefaultsPath.isEmpty()) {
        mLastError = QObject::tr("Couldn't find pglcmd's defaults path. Please set it in preferences.");
        return false;
    }
    
    
    qDebug() << mPglcmdDefaultsPath;
    QStringList data = getFileData(mPglcmdDefaultsPath);
    QString variable;

    foreach (QString line, data)
    {
        line = line.trimmed();
        if ( line.startsWith('#') ) //ignore comments
            continue;

        variable = getVariable(line);

        if( ! variable.isEmpty() )
            variables[variable] = PglSettings::getValueInLine(line);

    }


    //Overwrite the variables' values with the values from pglcmd.conf
    QString pglcmdConfPath(PglSettings::getStoredValue("CMD_CONF"));

    if ( pglcmdConfPath.isEmpty() ) {
        mLastError = QObject::tr("Couldn't find plgcmd's configuration path.");
        return false;
    }

    data = getFileData(pglcmdConfPath);
    foreach(QString line, data)
    {
        line = line.trimmed();
        if ( line.startsWith('#') ) //ignore comments
            continue;

        variable = getVariable(line);

        if ( variables.contains(variable) )
            variables[variable] = getValue(line);

    }
    
    mLastError = "";
    return true;
}

QHash<QString, QString> PglSettings::getVariables()
{
	return variables;
}

QString PglSettings::getStoredValue(const QString &variable)
{
	return variables[variable];
}

QString PglSettings::pglcmdDefaultsPath()
{
    return mPglcmdDefaultsPath;
}

QString PglSettings::lastError()
{
    return mLastError;
}
