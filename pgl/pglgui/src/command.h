#ifndef COMMAND_H
#define COMMAND_H

#include <QString>
#include <QMetaType>
#include <QList>

class Command
{
    QString mCommand;
    QString mOutput;
    bool mError;
    
    public:
        explicit Command(const QString& command="", const QString& output="", bool error = false);
        Command(const Command&);
        virtual ~Command();
        QString command() const;
        QString output() const;
        bool error() const;
        void setCommand(const QString&);
        void setOutput(const QString&);
        void setError(bool);
        bool contains(const QString&);
        
};

typedef QList<Command> CommandList;

Q_DECLARE_METATYPE(Command)
Q_DECLARE_METATYPE(CommandList)

#endif 
