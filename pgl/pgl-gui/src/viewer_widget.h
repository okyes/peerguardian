
#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QDialog>
#include <QTextEdit>
#include <QShowEvent>

class ViewerWidget : public QDialog
{
    Q_OBJECT
    
    QTextEdit* mTextView;
    
    public:
        explicit ViewerWidget(const QString& filePath="", QWidget* parent = 0);
        virtual ~ViewerWidget();
        
    protected:
        virtual void showEvent(QShowEvent *);
        
    private slots:
        void onFilterTextEdited(const QString&);
        void onReturnPressed();
        void moveScrollBarToBottom();
    
};

#endif
