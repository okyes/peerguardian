
#include "utils.h"
#include "file_transactions.h"
#include <QFile>
#include <QFileDialog>
#include <QMap>
#include <QVariant>
#include <QDir>
#include <QDebug>

static QList<Port> mPorts;
static QHash<QString, int> mPortsPair;

// **** PORT CLASS **** //
Port::Port(const QString& name, const QString& prot, int n)
{
    mNames << name;

    //the port number can be consider an alias too
    if ( n > 0 )
        mNames << QString::number(n);

    mProtocols << prot;
    mNumber = n;
}

Port::Port()
{
    mNumber = 0;
}

Port::Port(const Port& other)
{
    *this = other;
}

int Port::number() const
{
  return mNumber;
}

QStringList Port::protocols() const
{
  return mProtocols;
}

QStringList Port::names() const
{
  return mNames;
}

QString Port::name() const
{
    if ( ! mNames.isEmpty() )
        return mNames[0];
  
    return QString("");
}

bool Port::hasProtocol(const QString& protocol) const
{
  return mProtocols.contains(protocol, Qt::CaseInsensitive); 
}

void Port::addProtocols(const QStringList& protocols)
{
    mProtocols << protocols;
    mProtocols.removeDuplicates();
}

void Port::addName(const QString& name)
{
    if ( ! mNames.contains(name) )
        mNames << name;
}

/*Port& Port::operator=(const Port& other)
{
    m_number = other.number();
    m_protocols = other.protocols();
    m_values = other.values();
    
    return *this;
}*/

/*bool Port::operator==(WhitelistItem& item)
{
    foreach(const QString& name, mNames)
        if (item.value() == name)
            return true;

    return false;
}*/

bool Port::containsName(const QString& name) const
{
  foreach(const QString& _name, mNames)
    if (name == _name)
      return true;
    
  return false;
}

// ****  **** //

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

QString getValue(const QString& line)
{
    QString value("");

    if ( line.contains("=") )
    {
        value = line.split("=", QString::SkipEmptyParts)[1];

        if ( value == "\"\"" )
            return QString("");

        if ( value.size() > 2 && value.contains('"') )
            return value.split('"', QString::SkipEmptyParts)[0];

    }

    return value;
}

QString getVariable(const QString& line)
{
    if ( line.contains("=") )
         return line.split("=", QString::SkipEmptyParts)[0];

    return QString("");
}

QString getValue(const QString& path, const QString& search)
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
    QString searchLine("");
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
        {
            searchLine = line;
            break;
        }
	}

    file.close();
    return searchLine;
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
    QFile file;
    if ( ! fileInfo.isDir() )
        file.setFileName(fileInfo.absolutePath() + "/test_file");
    else
        file.setFileName(fileInfo.filePath() + "/test_file");

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


QStringList replaceValueInData(QStringList& data, const QString & variable, const QString & value)
{
    //this function is usually used to receive pglcmd.conf and check for variables and values there
    QRegExp re(QString("^%1=.*").arg(variable));
    int pos = data.indexOf(re);

    //Usual case: if the variable doesn't exist in pglcmd.conf and it's the same as in pglcmd.defaults
    //if ( pos == -1 && value == PglSetting::getStoredValue(variable))
        //return data;

    while ( pos != -1 )
    {
        data.removeAt(pos);
        pos = data.indexOf(re);
    }

    data << variable + QString("=\"") + value + QString('"');

    return data;
}


void replaceValueInFile(const QString& path, const QString & variable, const QString & value)
{

    QFile file( path );
    QStringList newData;
    QString line("");

    if ( path.isEmpty() ) {
		qWarning() << Q_FUNC_INFO << "Empty file path given, doing nothing";
		return;
	}
    else if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		qWarning() << Q_FUNC_INFO << "Could not read from file" << path;
		return;
	}

    QTextStream in( &file );
    QFileInfo fileInfo( path );
    bool found = false;

    while ( ! in.atEnd() )
    {
        line = in.readLine().trimmed();
        if ( line.contains(variable) )
        {
            found = true;
            newData << variable + QString("=\"") + value + QString('"');
        }
        else
            newData << line;
	}

    if ( ! found )
        newData << variable + QString("=\"") + value + QString('"');

    saveFileData(newData, "/tmp/" + fileInfo.fileName());

    file.close();
}


QString getFileName(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.fileName();
}

bool hasValueInData(const QString& value, const QStringList& data)
{
    foreach(QString line, data)
        if ( line.contains(value) )
            return true;

    return false;
}


int confirm(QString title, QString msg, QWidget *parent)
{
       int confirm = QMessageBox::warning( 
        parent, 
        title,
	msg,
	QMessageBox::Yes, QMessageBox::No
        );
       
       return confirm;
}

bool isNumber(const QString& str)
{
    for (int i=0; i < str.size(); i++)
      if (! str[i].isDigit())
          return false;

    return true;
}

bool isPort(QString & p)
{
    if ( p.contains(":") ) //port range
    {
        QStringList ports = p.split(":");
        
        if ( ports.size() > 2 )
            return false;
        
        foreach(QString port, ports)
            if (! isNumber(port))
                return false;
                
        return true;
    }
    
    
    if (! isNumber(p))
      return false;

    if (port(p.toLower()) == -1)
      return false;

    return true;
}

Port getPortFromLine(QString line)
{
    QStringList elements;
    int portNum;
    QString protocol;
    Port port;

    line = line.simplified();

    if ( line.isEmpty() || line.startsWith("#") )
        return Port();

    elements = line.split(" ");

    portNum = elements[1].split("/")[0].toInt();
    protocol = elements[1].split("/")[1];

    port = Port(elements[0], protocol, portNum);

    if ( elements.size() >= 3 && ( ! elements[2].startsWith("#")) )
        port.addName(elements[2]);

    return port;
}

void loadPorts()
{   
    if (! mPorts.isEmpty())
      return;
    
    QStringList fileData = getFileData("/etc/services");
    QStringList elements;
    int portNum;
    QString protocol;
    Port port1, port2;

    //old way for backwards compatibility
    /*foreach(QString line, fileData)
    {
        line = line.simplified();

        if ( line.isEmpty() || line.startsWith("#") )
            continue;

        elements = line.split(" ");

        portNum = elements[1].split("/")[0].toInt();
        protocol = elements[1].split("/")[1];

        mPortsFilter[elements[0]] = portNum;

        //check for alternative names
        if ( elements.size() >= 3 && ( ! elements[2].startsWith("#")) )
            mPortsFilter[elements[2]] = portNum;
    }*/

    for ( int i=0; i < fileData.size(); i++ )
    {
        port1 = getPortFromLine(fileData[i]);

        if ( i < fileData.size()-1 )
        {
            //get next line
            port2 = getPortFromLine(fileData[i+1]);

            if ( port1.name() == port2.name() )
            {
                port1.addProtocols(port2.protocols());
                ++i; //ignores next line
            }
        }

        mPorts.append(port1);
    }
}
  
int port(const QString& name) 
{
  if (mPorts.isEmpty())
    loadPorts();
  
  if (isNumber(name))
    return name.toInt();
  
  foreach(const Port& port, mPorts) 
      if (port.containsName(name))
        return port.number();
  
  return -1;
}
  
QList<Port> ports()
{
  if (mPorts.isEmpty())
    loadPorts();
  return mPorts;
}

QHash<QString, int> portNamesAndNumbersPair()
{
  QHash<QString, int> ports;
  
  foreach(const Port& port, mPorts)
    foreach(const QString& name, port.names()) 
      if (! isNumber(name))
        ports[name] = port.number();
 
  return ports;
}
