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
#include "file_transactions.h"
#include "utils.h"
#include <QDebug>


QHash<QString, QString> PglSettings::variables = QHash<QString, QString>();


QString PglSettings::getVariableInValue(QString & var)
{
    
    if ( var.contains("$") )
    {
        QString stripvar(var);
        int n = 0;
        
        if ( var.contains("{") && var.contains("}") )
        {
            int pos1 = var.indexOf('{') + 1;
            n = var.indexOf('}') - pos1;
            stripvar = var.mid(pos1, n);
            n = 3;
        }
        else
        {
            int pos1 = var.indexOf('$') + 1;
            stripvar = var.mid(pos1);
            n = 1;
        }
        
        if ( PglSettings::variables.contains(stripvar) )
            return var.replace(var.indexOf("$"), stripvar.size()+n, variables[stripvar]);
    }


    return var;
}

QString PglSettings::getValueInLine(QString& line)
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
    
    
    //qDebug() << line << "<<>>" <<newValue;
    return newValue;
}

void PglSettings::loadSettings()
{
    QStringList data = getFileData(PGLCMD_DEFAULTS_PATH);
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
    
    QString pglcmdConfPath(""); 
    
    foreach(QString value, variables.values() )
    {
        if ( value.contains("pglcmd.conf") )
        {
            pglcmdConfPath = value;
            break;
        }
    }
    
    if ( pglcmdConfPath.isEmpty() )
        return;
        
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
    
    foreach(QString key, variables.keys() )
        qDebug() << key << ": " << variables[key];
    
}
