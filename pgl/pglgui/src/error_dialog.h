
#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H

#include <QDialog>
#include <QTreeWidget>

#include "command.h"

class ErrorDialog : public QDialog
{
    Q_OBJECT

    QTreeWidget* mErrorWidget;

    public:
        explicit ErrorDialog(const CommandList& commands, QWidget* parent = 0);
        virtual ~ErrorDialog();
    private slots:

};

#endif

