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

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "process_dialog.h"

ProcessDialog::ProcessDialog(QWidget* parent) :
    QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
    mMessageBox = 0;
    setModal(true);
    setWindowTitle(tr("A process is running"));
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* hlayout = new QHBoxLayout();
    layout->addLayout(hlayout);

    mProgressBar = new QProgressBar(this);
    mProgressBar->setMaximum(0);
    hlayout->addWidget(mProgressBar);

    mCancelButton = new QPushButton(tr("Cancel"), this);
    hlayout->addWidget(mCancelButton);
    connect(mCancelButton, SIGNAL(clicked()), this, SLOT(onCancelButtonClicked()));

    mShowDetailsButton = new QPushButton(tr("Show Details"), this);
    mShowDetailsButton->setCheckable(true);
    mShowDetailsButton->setChecked(false);
    layout->addWidget(mShowDetailsButton);

    mOutputEdit = new QTextEdit(this);
    mOutputEdit->setReadOnly(true);
    layout->addWidget(mOutputEdit);
    mOutputEdit->hide();

    connect(mShowDetailsButton, SIGNAL(toggled(bool)), this, SLOT(onShowDetailsToggled(bool)));
    connect(this, SIGNAL(finished(int)), this, SLOT(onFinished(int)));

    resize(350, height());
}

ProcessDialog::~ProcessDialog()
{
}

void ProcessDialog::writeOutput(const QByteArray & data)
{
    QString output = mOutputEdit->toPlainText();
    output += QString(data);
    mOutputEdit->setPlainText(output);
}

void ProcessDialog::start(QObject* receiver, const char* member)
{
    mOutputEdit->clear();
    mShowDetailsButton->setChecked(false);
    this->disconnect(SIGNAL(canceled()));
    if (receiver && member)
        connect(this, SIGNAL(canceled()), receiver, member);
    open();
}

void ProcessDialog::onShowDetailsToggled(bool show)
{
    if (show) {
        mOutputEdit->setVisible(show);
    }
    else {
        mOutputEdit->setVisible(show);
        if (!isMaximized()) {
            adjustSize();
        }
    }
}

QSize ProcessDialog::sizeHint() const
{
    QSize size = QDialog::sizeHint();
    if (mOutputEdit->isHidden()) {
        size = geometry().size();
        size.setHeight(mProgressBar->height() + mShowDetailsButton->height());
    }

    return size;
}

void ProcessDialog::onCancelButtonClicked()
{
   if (!mMessageBox) {
        mMessageBox = new QMessageBox(QMessageBox::Warning, tr("Cancel running process?"),
                                 tr("Are you sure you want to cancel the current running process?"),
                                 QMessageBox::Yes | QMessageBox::No, this);
        mMessageBox->setDefaultButton(QMessageBox::No);
   }

   int result = mMessageBox->exec();

   if (result == QMessageBox::Yes)
       emit canceled();
}

void ProcessDialog::onFinished(int result)
{
    if (mMessageBox) {
        mMessageBox->reject();
        delete mMessageBox;
        mMessageBox = 0;
    }
}

void ProcessDialog::keyPressEvent(QKeyEvent *e) {
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}
