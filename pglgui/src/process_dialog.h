/******************************************************************************
 *   Copyright (C) 2017 by Carlos Pais <fr33mind@users.sourceforge.net>       *
 *                                                                            *
 *   This file is part of pgl.                                                *
 *                                                                            *
 *   pgl is free software: you can redistribute it and/or modify              *
 *   it under the terms of the GNU General Public License as published by     *
 *   the Free Software Foundation, either version 3 of the License, or        *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   pgl is distributed in the hope that it will be useful,                   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU General Public License for more details.                             *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License        *
 *   along with pgl.  If not, see <http://www.gnu.org/licenses/>.             *
 *****************************************************************************/


#ifndef PROCESS_DIALOG_H
#define PROCESS_DIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRect>
#include <QMessageBox>
#include <QKeyEvent>

class ProcessDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit ProcessDialog(QWidget* parent = 0);
        virtual ~ProcessDialog();
        virtual QSize sizeHint() const;

    protected:
        virtual void keyPressEvent(QKeyEvent*);

    public slots:
        void writeOutput(const QByteArray&);
        void start(QObject* receiver=0, const char* member=0);

    private slots:
        void onShowDetailsToggled(bool);
        void onCancelButtonClicked();
        void onFinished(int);

    signals:
        void canceled();

    private:
        QTextEdit* mOutputEdit;
        QProgressBar* mProgressBar;
        QPushButton* mShowDetailsButton;
        QPushButton* mCancelButton;
        QMessageBox* mMessageBox;
};

#endif


