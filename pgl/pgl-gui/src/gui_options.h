
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
    QList<QTreeWidgetItem*> m_Blocklist;
    QList<QTreeWidgetItem*> m_Whitelist;
    QList<QTreeWidgetItem*> m_WhitelistItemsForIptablesRemoval;
    int m_WhitelistItemsRemoved;

    public:
        GuiOptions();
        GuiOptions(Peerguardian * gui);
        ~GuiOptions();
        void addWindow(Peerguardian * gui) { m_Window = gui; }
        bool isChanged();
        bool isChanged(QTreeWidgetItem *);
        void updateUpdateRadioBtn();
        void updateStartAtBoot();
        void update();
        QRadioButton* getActiveUpdateRadioButton();
        QString getActiveUpdateRadioButtonName();
        bool whitelistStateChanged(bool anyChange = true);
        bool blocklistStateChanged();
        bool hasToUpdatePglcmdConf();
        bool hasToUpdateBlocklistList();
        bool hasCheckboxChanged(QCheckBox*);
        bool hasRadioButtonChanged(QRadioButton*);
        Qt::CheckState getState(int);
        void deleteItems(QList<QTreeWidgetItem*>&);
        /*void undoBlocklist();
        void undoWhitelist();*/
        void undo();
        int getWhitelistPrevSize() {return m_WhitelistState.size(); }
        int getPositionFirstAddedWhitelistItem();
        void addWhitelistItemForIptablesRemoval(QTreeWidgetItem* item);
        void addRemovedWhitelistItem(QTreeWidgetItem *);
        QList<QTreeWidgetItem*>& getRemovedWhitelistItemsForIptablesRemoval(){ return m_WhitelistItemsForIptablesRemoval; }
        void updateBlocklist();
        void updateWhitelist(int starFrom = 0);
};


#endif
