#include <QString> 
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

QString getValidPath(const QString &path, const QString &defaultPath );
QStringList selectFiles(QWidget * parent=0, QString title="Select one or more Blocklists", QString startPath=QDir::homePath());
QString getVariable(QString&);
QString getVariable(const QString&, const QString&);
QString getLineWith(const QString&, const QString&);
bool isValidIp(const QString &text );
bool isPort(const QString & );

