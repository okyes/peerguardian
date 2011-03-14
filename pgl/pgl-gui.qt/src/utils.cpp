
#include "utils.h"
#include <QFile>
#include <QFileDialog>
#include <QMap>

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

QStringList selectFiles(QWidget * parent, QString title, QString startPath)
{
    QString filter;
    filter += "All Suported files  (*.p2p *.gz *.7z *.zip *.dat );;";
    filter += "Peerguardian P2P format (*.p2p);;zip (*.zip);; 7z (*.7z);;GNU zip (*.gz);;dat (*.dat)";
    
     QStringList files = QFileDialog::getOpenFileNames(
                         parent,
                         title,
                         startPath,
                         filter);

    return files;
}

QString getVariable(QString& line)
{
    if ( line.contains("=") )
        line = line.split("=", QString::SkipEmptyParts)[1];
    if ( line.contains('"') )
        line = line.split('"', QString::SkipEmptyParts)[0];
    
    return line;
}

QString getVariable(const QString& path, const QString& search)
{
    QString line = getLineWith(path, search);
    if ( line.contains("=") )
        line = line.split("=", QString::SkipEmptyParts)[1];
    if ( line.contains('"') )
        line = line.split('"', QString::SkipEmptyParts)[0];
    
    return line;
}

QString getLineWith(const QString& path, const QString& search)
{	
    QFile file( path );
    QString line ("");
	if ( path.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Empty file path given, doing nothing";
		return line;
	}
    else if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		qWarning() << Q_FUNC_INFO << "Could not read from file" << path;
		return line;
	}
    
    QTextStream in( &file );
    
	while ( ! in.atEnd() ) 
    {
        line = in.readLine().trimmed();
        if ( line.contains(search) )
            return line;
	}
    
    return QString("");
}

