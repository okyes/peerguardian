
#include "utils.h"
#include <QFile>
#include <QFileDialog>
#include <QMap>
#include <QVariant>
#include <QDir>
#include <QDebug>

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

QStringList selectFiles(QWidget * parent, QString filter, QString title, QString startPath)
{    
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

bool isValidIp( const QString &text ){

	QString ip = text.trimmed();

	if ( ip.isEmpty() ) {
		return false;
	}
	else {
		//Split the string into two sections
		//For example the string 127.0.0.1/24 will be split into two strings:
		//mainIP = "127.0.0.1"
		//range = "24"
		QVector< QString > ipSections = QVector< QString >::fromList( ip.split( "/" ) );
		if ( ipSections.size() < 1 || ipSections.size() > 2 ) {
			return false;
		}
		QString mainIp = ipSections[0];
		QString range = ( ipSections.size() == 2 ) ? ipSections[1] : QString();
		//Split the IP address 
		//E.g. split 127.0.0.1 to "127", "0", "0", "1"
		QVector< QString > ipParts = QVector<QString>::fromList( mainIp.split( "." ) );
		//If size != 4 then it's not an IP
		if ( ipParts.size() != 4 ) {
			return false;
		}
		
		for ( int i = 0; i < ipParts.size(); i++ ) {
			if ( ipParts[i].isEmpty() ) {
				return false;
			}
			//Check that every part of the IP is a positive  integers less or equal to 255
			if ( QVariant( ipParts[i] ).toInt() > 255 || QVariant( ipParts[i] ).toInt() < 0 ) {
				return false;
			}
			for ( int j = 0; j < ipParts[i].length(); j++ ) {
				if ( !ipParts[i][j].isNumber() ) {
					return false;
				}
			}
		}
		//Check if the range is a valid subnet mask
		if ( !isValidIp( range ) ) {
			//Check that the range is a positive integer less or equal to 24
			if ( QVariant( range ).toInt() <= 24 && QVariant( range ).toInt() >= 0 ) {
				for ( int i = 0; i < range.length(); i++ ) {
					if ( !range[i].isNumber() ) {
						return false; 
					}
			}
		}
			else {
				return false;
			}
		}
	}
		

	return true;

}

bool isPort(const QString & p )
{

    if ( p == QString::number(p.toInt()) )
        return true;
        
    QMap<QString, int> ports;
    ports["HTTP"] = 80;
    ports["HTTPS"] = 443;
    ports["FTP"] = 21;
    ports["POP"] = 110;
    ports["SMTP"] = 587;
    ports["IMAP"] = 143;
    ports["SSH"] = 22;
    
    if ( ports.contains(p.toUpper()) )
        return true;
    
    return false;
}

QFileInfoList getFilesInfo(QString & dir)
{
    QDir directory(dir);
    
    return directory.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
}

QString getPointer(QString & dir, QString & filepathPointed)
{
    foreach(QFileInfo fileInfo, getFilesInfo(dir) )
        if ( fileInfo.isSymLink() )
            if ( fileInfo.symLinkTarget() == filepathPointed )
                return fileInfo.absoluteFilePath();
    
    return "";
}

bool isPointingTo(QString & dir, QString & filepathPointed)
{
        if ( ! getPointer(dir, filepathPointed).isEmpty() )
            return true;
        
        return false;
}

QString getNewFileName(QString dir, const QString name)
{
    if ( dir.isEmpty() )
        return dir;
    
    QDir directory(dir);
    if ( ! directory.exists() )
        return QString();
        
    
    int counter = 0;
    QString temp_name = name;
    
    while(1)
    {
        if ( ! directory.exists(temp_name) )
            return dir + "/" + temp_name;
        
        counter++;
        temp_name = name + "_" + QString::number(counter);
    }
    
    return QString();
    
}

bool hasPermissions(const QString & filepath)
{
    
    if ( filepath.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Empty file path given, doing nothing";
		return false;
	}
 
    QFileInfo fileInfo (filepath);
    QString path = fileInfo.absolutePath() + "/";
    QFile file(path + "test_file");
    
    qDebug() << file.fileName();
    
    if ( ! file.open( QIODevice::ReadWrite | QIODevice::Text ) ) {
		qWarning() << Q_FUNC_INFO << "Could not read from file" << file.fileName();
		return false;
	}
    
    file.close();
    
    return true;
}

QString joinPath(const QString & dir, const QString & file)
{
    if ( dir.isEmpty() )
        return file;
    
    QString directory = dir.trimmed();
    
    if (directory[directory.size()-1] != '/')
        directory += "/";
        
    return directory + file;
    
}

