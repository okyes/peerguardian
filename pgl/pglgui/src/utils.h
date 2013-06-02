#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfoList>
#include <QMessageBox>
#include <QRegExp>

QString getValidPath(const QString &path, const QString &defaultPath );
QStringList selectFiles(QWidget * parent=0, QString filter = "", QString title="Select one or more Blocklists", QString startPath=QDir::homePath());
QString getValue(const QString&);
QStringList getValues(const QString&);
QString getVariable(const QString&);
QString getValue(const QString&, const QString&);
QString getLineWith(const QString&, const QString&);
bool isValidIp(const QString &text );
QFileInfoList getFilesInfo(const QString &, QDir::Filters filters=QDir::QDir::NoDotAndDotDot|QDir::Files);
bool isPointingTo(QString &, QString &);
QString getPointer(const QString &, const QString &);
bool hasPermissions(const QString&);
QString getNewFileName(QString dir, const QString name);
QString joinPath(const QString& dir, const QString& file);
void replaceValueInFile(const QString& path, const QString & variable, const QString & value);
void setValueInData(QStringList& data, const QString & variable, const QString & value);
QStringList cleanData(QStringList& data);
QString getFileName(const QString& path);
bool hasValueInData(const QString&, const QStringList&);
int confirm(QString title, QString msg, QWidget *parent=NULL);
bool isNumber(const QString&);

#endif
