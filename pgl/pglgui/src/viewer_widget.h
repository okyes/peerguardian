
#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QDialog>
#include <QTextEdit>
#include <QShowEvent>
#include <QAbstractButton>
#include <QDialogButtonBox>

class ViewerWidget : public QDialog
{
    Q_OBJECT
    
    QTextEdit* mTextView;
    QDialogButtonBox* mButtonBox;
    
    public:
        explicit ViewerWidget(const QString& info="", QWidget* parent = 0);
        virtual ~ViewerWidget();
        
    protected:
        virtual void showEvent(QShowEvent *);
        
    private slots:
        void onFilterTextEdited(const QString&);
        void onReturnPressed();
        void moveScrollBarToBottom();
        
    protected:
        void keyPressEvent ( QKeyEvent * e );
    
};

#endif
