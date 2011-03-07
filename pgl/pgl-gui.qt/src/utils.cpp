
#include "utils.h"
#include <QFile>

QString getValidPath(const QString &path, const QString &defaultPath )
{
    QString new_path;
    
    if ( ! path.isEmpty() && QFile::exists(path) )
		new_path = path;
	else if ( QFile::exists(defaultPath) )
		new_path = defaultPath;
	else 
        new_path = "";
    
    return new_path;
}
