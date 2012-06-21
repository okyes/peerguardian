
#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QDialog>
#include <QTextEdit>
#include <QShowEvent>
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QThread>
#include <QFile>
#include <QLineEdit>

#include "file_transactions.h"

#define MAX_LINE_COUNT 5000

class ReadFile : public QThread
{
    Q_OBJECT
    
    QString mData;
    QString mPath;
    
    public:
        explicit ReadFile(const QString& path, QObject* parent = 0)
        {
            mPath = path;
            mData = "";
        }
    
        inline void run() {
            QStringList data;
            if (QFile::exists(mPath))
                data = getFileData(mPath);
                
            if (data.size() > MAX_LINE_COUNT) {
                int dif = data.size() - MAX_LINE_COUNT;
                data = data.mid(dif);
            }
            
            mData = data.join("\n");
        }
        
        inline QString data() {
            return mData;
        }
};

class ViewerWidget : public QDialog
{
    Q_OBJECT
    
    QTextEdit* mTextView;
    QDialogButtonBox* mButtonBox;
    QLineEdit *mFilterEdit;
    
    public:
        explicit ViewerWidget(const QString& info="", QWidget* parent = 0);
        virtual ~ViewerWidget();
        
    protected:
        virtual void showEvent(QShowEvent *);
        
    private slots:
        void onFilterTextEdited(const QString&);
        void onReturnPressed();
        void moveScrollBarToBottom();
        void onReadFileFinished();
        
    protected:
        void keyPressEvent ( QKeyEvent * e );
    
};

#endif
