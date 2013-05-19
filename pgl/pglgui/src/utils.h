#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfoList>
#include <QMessageBox>

class Port
{
    int mNumber;
    QStringList mProtocols;
    QStringList mNames;

    public:
        Port();
        Port(const Port& other);
        Port(const QString& desig, const QString& prot, int n=0);
        ~Port(){};
        void addProtocols(const QStringList&);
        void addName(const QString&);
        bool containAlias(const QString&);
        int number() const;
        QStringList names() const;
        QString name() const;
        bool containsName(const QString&) const;
        QStringList protocols() const;
        //Port& operator=(const Port& other);
        bool hasProtocol(const QString&) const;
        //bool operator==(WhitelistItem& item);
        bool operator==(const Port& );
};

QString getValidPath(const QString &path, const QString &defaultPath );
QStringList selectFiles(QWidget * parent=0, QString filter = "", QString title="Select one or more Blocklists", QString startPath=QDir::homePath());
QString getValue(const QString&);
QStringList getValues(const QString&);
QString getVariable(const QString&);
QString getValue(const QString&, const QString&);
QString getLineWith(const QString&, const QString&);
bool isValidIp(const QString &text );
QFileInfoList getFilesInfo(const QString &);
bool isPointingTo(QString &, QString &);
QString getPointer(const QString &, const QString &);
bool hasPermissions(const QString&);
QString getNewFileName(QString dir, const QString name);
QString joinPath(const QString& dir, const QString& file);
void replaceValueInFile(const QString& path, const QString & variable, const QString & value);
QStringList replaceValueInData(QStringList& data, const QString & variable, const QString & value);
QString getFileName(const QString& path);
bool hasValueInData(const QString&, const QStringList&);
int confirm(QString title, QString msg, QWidget *parent=NULL);
bool isPort(QString&);
bool isNumber(const QString&);
Port getPortFromLine(QString);
int portNumber(const QString&);
QList<Port> ports();
QHash<QString, int> portNamesAndNumbersPair();

#endif
