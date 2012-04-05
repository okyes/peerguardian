#include "command.h"

#include <QMetaType>

Command::Command(const QString& command, const QString& output, bool error)
{
    mCommand = command;
    mOutput = output;
    mError = error;
}

Command::Command(const Command& other)
{
    *this = other;
}

Command::~Command()
{
}

QString Command::command() const
{
    return mCommand;
}

QString Command::output() const
{
    return mOutput;
}

bool Command::error() const
{
    return mError;
}


void Command::setCommand(const QString& command)
{
    mCommand = command;
}

void Command::setOutput(const QString& output)
{
    mOutput = output;
}

void Command::setError(bool error)
{
    mError = error;
}

bool Command::contains(const QString& str)
{
    return mCommand.contains(str);
}
