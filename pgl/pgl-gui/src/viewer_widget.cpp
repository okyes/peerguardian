 
#include "viewer_widget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QFile>
#include <QList>
#include <QDebug>

#include "file_transactions.h"

ViewerWidget::ViewerWidget(const QString& filePath, QWidget* parent) :
    QDialog(parent)
{
    QString text("");
    QString notFound("");
    bool fileExists = true;
    
    if (QFile::exists(filePath)) 
        text = getFileData(filePath).join("\n");
    else
        fileExists = false;
        
    QVBoxLayout* vlayout = new QVBoxLayout(this);
    //QHBoxLayout* hlayout = new QHBoxLayout;
    //vlayout->addLayout(hlayout);
    QLineEdit * filter = new QLineEdit(this);
    mTextView = new QTextEdit(this);
    mTextView->setReadOnly(true);
    mTextView->setText(text);
    
    connect(filter, SIGNAL(textEdited(const QString&)), this, SLOT(onFilterTextEdited(const QString&)));
    connect(filter, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));

    vlayout->addWidget(mTextView);
    vlayout->addWidget(filter);
    
    resize(500, 300);
    setWindowTitle(filePath + notFound);
    if (! fileExists) {
        setWindowTitle( windowTitle() + tr(" (File doesn't exist)"));
        mTextView->setDisabled(true);
        filter->setDisabled(true);
    }
}

ViewerWidget::~ViewerWidget()
{
}

void ViewerWidget::onFilterTextEdited(const QString& text)
{
    QString colorStartTag("<span style=\"background-color:yellow;\">");
    QString colorEndTag("</span>");
    QString plainText = mTextView->toPlainText();
    QStringList lines;
    QString txt;
    int index;
    
    if (plainText.contains("\n"))
        lines = plainText.split("\n");
    else if (plainText.contains("<br/>"))
        lines = plainText.split("<br/>");
    else 
        lines << plainText;

    bool changed = false;
    for(int i=0; i < lines.size(); i++) {
        index = lines[i].indexOf(text, 0, Qt::CaseInsensitive);
        if (index != -1) {
            txt = lines[i].mid(index, text.size());
            lines[i].replace(txt, colorStartTag + txt + colorEndTag);
            changed = true;
        }
    }
    
    QLineEdit *filter = qobject_cast<QLineEdit*>(sender());
    
    if (changed) {
        mTextView->setText(lines.join("<br/>"));
        if (filter)
            filter->setStyleSheet("");
        
        index = plainText.indexOf(text, 0, Qt::CaseInsensitive);

        while(mTextView->textCursor().position() < index) 
            mTextView->moveCursor(QTextCursor::Down);
    }
    else{
        if (filter)
            filter->setStyleSheet("background-color:red;");
    }
}

void ViewerWidget::onReturnPressed()
{
    QLineEdit *filter = qobject_cast<QLineEdit*>(sender());
    if (! filter)
        return;
    
    QString text = mTextView->toPlainText();
    QString filterText = filter->text();
    QTextCursor cursor = mTextView->textCursor();
    QStringList lines;
    int from=0, index=0;
    
    from = mTextView->textCursor().position();
    index = text.indexOf(filterText, from, Qt::CaseInsensitive);
    
    if (index == -1)
        return;
    
    while(mTextView->textCursor().position() < index) 
        mTextView->moveCursor(QTextCursor::Down);
}