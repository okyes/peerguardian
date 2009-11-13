/**************************************************************************
*   Copyright (C) 2009-2010 by Dimitris Palyvos-Giannas                   *
*   jimaras@gmail.com                                                     *
*                                                                         *
*   This is a part of pgl-gui.                                            *
*   Pgl-gui is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include <QObject>
#include <QThread>
#include <QProcess>

#include <QString>

#include "rawdata.h"

/**
 * @brief A class used to handle processes
 * 
 * ProcessHandler creates a new QProcess and runs it. 
 * The main program does not have to wait for the process to finish its execution.
 * Moreover, the class saves and gives the output of the process when it's requested.
 */

class ProcessHandler : public QThread, public RawData {

    Q_OBJECT

    public:
        /**
         * Default constructor.
         * 
         * Creates an empty object.
         * @param parent1 The QThread parent of this object.
         * @param parent2 The RawData parent of this object.
         */
        ProcessHandler( QThread *parent1 = 0, RawData *parent2 = 0 );
        /**
         * Alternative constructor.
         * 
         * Creates an empty object.
         * @param mode The mode which will be used the next time a process is started.
         * @param parent1 The QThread parent of this object.
         * @param parent2 The RawData parent of this object.
         */
        ProcessHandler( const QProcess::ProcessChannelMode &mode, QThread *parent1 = 0, RawData *parent2 = 0 );
        /**
         * Destructor
         */
        virtual ~ProcessHandler();
        /**
         * Get the name of the process currently set using Open();
         *
         * @return The name of the process, including the arguments.
         */
        QString GetProcessName() const;
        /**
        * Sets the channel mode of the QProcess standard output and standard error channels to the mode specified.
        * 
        * The default constructor sets the mode as QProcess::SeparateChannels.
        * @param mode The mode which will be used the next time a process is started.
        */
        void SetChannelMode( const QProcess::ProcessChannelMode &mode );

    public slots:
        /**
         * Start a new process.
         * 
         * Creates a new QProcess and runs it, saving its output.
         * @param process A QString containing both the process name and the arguments.
         */
        virtual bool Open( const QString &process );
        /**
         * Terminates the thread.
         * 
         * Calling this is VERY dangerous. Should be used only in when absolutely necessary.
         * @return True if the Thread was terminated sucessfully, otherwise false.
         */
        virtual bool Close();
        /**
         * Run the process set using open and emit its output using the appropriate signals when it has finished running.
         */
        virtual void RequestNewData();

    private:
        QString m_ProcessName;
        QProcess m_Runner;
        QProcess::ProcessChannelMode m_ChanMode;

};

#endif //PROCESSHANDLER_H