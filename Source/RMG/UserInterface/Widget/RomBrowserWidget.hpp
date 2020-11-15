/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROMBROWSERWIDGET_HPP
#define ROMBROWSERWIDGET_HPP

#include "../../Globals.hpp"
#include "../../Thread/RomSearcherThread.hpp"
#include "../NoFocusDelegate.hpp"

#include <QHeaderView>
#include <QList>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>

#define MAX_ROM_INFO 50

namespace UserInterface
{
namespace Widget
{
class RomBrowserWidget : public QTableView
{
    Q_OBJECT

  public:
    RomBrowserWidget(QWidget *);
    ~RomBrowserWidget(void);

    void RefreshRomList(void);

    void SetDirectory(QString);

  private:
    QString directory;

    QStandardItemModel *model_Model;
    QStringList model_LabelList;
    void model_Init(void);
    void model_Setup(void);
    void model_LabelList_Setup(void);

    NoFocusDelegate *widget_Delegate;
    void widget_Init(void);

    Thread::RomSearcherThread *rom_Searcher_Thread;
    bool rom_Searcher_Recursive;
    bool rom_Searcher_MaxItems;
    bool rom_Searcher_Running;

    void rom_Searcher_Init(void);

    QList<M64P::Wrapper::RomInfo_t> rom_List;
    int rom_List_Index;

    bool rom_List_Recursive;
    bool rom_List_Fill_Thread_Running = false;
    void rom_List_Init();
    void rom_List_Fill(QString);

    void column_SetSize();

  public slots:
    void on_Row_DoubleClicked(const QModelIndex &);
    void on_RomBrowserThread_Received(M64P::Wrapper::RomInfo_t info);
    void on_RomBrowserThread_Finished(void);

  signals:
    void on_RomBrowser_Select(QString);
};
} // namespace Widget
} // namespace UserInterface

#endif // ROMBROWSERWIDGET_HPP
