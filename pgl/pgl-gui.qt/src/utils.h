#ifndef UTILS_H
#define UTILS_H

#include <QString> 
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfoList>


QString getValidPath(const QString &path, const QString &defaultPath );
QStringList selectFiles(QWidget * parent=0, QString filter = "", QString title="Select one or more Blocklists", QString startPath=QDir::homePath());
QString getVariable(QString&);
QString getVariable(const QString&, const QString&);
QString getLineWith(const QString&, const QString&);
bool isValidIp(const QString &text );
bool isPort(const QString & );
QFileInfoList getFilesInfo(QString &);
bool isPointingTo(QString &, QString &);
QString getPointer(QString &, QString &);
bool hasPermissions(const QString&);
QString getNewFileName(QString dir, const QString name);
QString joinPath(const QString& dir, const QString& file);
void replaceValueInFile(const QString& path, const QString & variable, const QString & value);
QString getFileName(const QString& path);


#endif
