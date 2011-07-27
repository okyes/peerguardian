
#ifndef GUI_OPTIONS_H
#define GUI_OPTIONS_H

#include <QList>
#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <QDebug>
#include <QCheckBox>
#include <QRadioButton>
#include "peerguardian.h"

class Peerguardian;

class GuiOptions
{
    Peerguardian * m_Window;
    QString updateRadioBtnName;
    QRadioButton * updateRadioBtn;
    bool startAtBoot;
    bool updateBlocklistsAutomatically;
    QList<int> m_BlocklistState;
    QList<int> m_WhitelistState;

    public:
        GuiOptions() { m_Window = NULL; }
        GuiOptions(Peerguardian * gui) { m_Window = gui; }
        ~GuiOptions(){ qDebug() << "~GuiOptions()"; }
        void addWindow(Peerguardian * gui) { m_Window = gui; }
        bool isChanged();
        bool isChanged(QTreeWidgetItem *);
        void updateUpdateRadioBtn();
        void updateStartAtBoot();
        void update();
        QRadioButton* getActiveUpdateRadioButton();
        QString getActiveUpdateRadioButtonName();
        bool listStateChanged(QTreeWidget * tree);
        void updateList(QTreeWidget * tree);
        bool hasToUpdatePglcmdConf();
        bool hasToUpdateBlocklistList();
        bool hasCheckboxChanged(QCheckBox*);
        bool hasRadioButtonChanged(QRadioButton*);
        Qt::CheckState getState(int);
        void undoBlocklist();
        void undoWhitelist();
        void undo();
        
};


#endif
