
#ifndef GUI_OPTIONS_H
#define GUI_OPTIONS_H

#include <QList>
#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <QDebug>
#include "peerguardian.h"

class Peerguardian;
 
class GuiOptions
{
    Peerguardian * m_Window;
    QString updateRadioBtn;
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
        void updateUpdateRadioBtn();
        void updateStartAtBoot();
        void update();
        QString getActiveUpdateRadioButton();
        bool listStateChanged(QTreeWidget * tree);
        void updateList(QTreeWidget * tree);
        bool hasToUpdatePglcmdConf();
        bool hasToUpdateBlocklistList();
};


#endif
