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

#include "Thread/RomSearcherThread.hpp"
#include "UserInterface/NoFocusDelegate.hpp"

#include "RomBrowserListViewWidget.hpp"
#include "RomBrowserGridViewWidget.hpp"
#include "RomBrowserLoadingWidget.hpp"
#include "RomBrowserEmptyWidget.hpp"

#include <QHeaderView>
#include <QList>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>
#include <QMenu>
#include <QAction>
#include <QGridLayout>
#include <QListWidget>
#include <QStackedWidget>

// forward declaration of internal struct
struct RomBrowserModelData;

namespace UserInterface
{
namespace Widget
{
class RomBrowserWidget : public QStackedWidget
{
    Q_OBJECT

  public:
    RomBrowserWidget(QWidget *);
    ~RomBrowserWidget(void);

    void RefreshRomList(void);
    bool IsRefreshingRomList(void);
    void StopRefreshRomList(void);

    void ShowList(void);
    void ShowGrid(void);

    void SetGridViewUniformSizes(bool value);

  private:
    Widget::RomBrowserEmptyWidget*    emptyWidget    = nullptr;
    Widget::RomBrowserLoadingWidget*  loadingWidget  = nullptr;

    Widget::RomBrowserListViewWidget* listViewWidget = nullptr;
    QStandardItemModel* listViewModel                = nullptr;
    Widget::RomBrowserGridViewWidget* gridViewWidget = nullptr;
    QStandardItemModel* gridViewModel                = nullptr;

    QWidget* currentViewWidget = nullptr;

    QElapsedTimer romSearcherTimer;
    Thread::RomSearcherThread* romSearcherThread = nullptr;

    bool sortRomResults = false;
    
    int listViewSortSection = 0;
    int listViewSortOrder = 0;

    QStringList columnNames;

    QMenu*   contextMenu;
    QAction* action_PlayGame;
    QAction* action_PlayGameWith;
    QAction* action_RefreshRomList;
    QAction* action_OpenRomDirectory;
    QAction* action_ChangeRomDirectory;
    QAction* action_RomInformation;
    QAction* action_EditGameSettings;
    QAction* action_EditCheats;
    QAction* action_ResetColumnSizes;
    QAction* action_SetCoverImage;
    QAction* action_RemoveCoverImage;

    QMenu*   menu_Columns;
    QAction* action_ColumnsMenuEntry;

    QString coversDirectory;

    QStandardItemModel* getCurrentModel(void);
    QAbstractItemView*  getCurrentModelView(void);
    bool getCurrentData(RomBrowserModelData& data);

    QString getCurrentRom(void);

    QIcon getCurrentCover(QString file, CoreRomHeader header, CoreRomSettings settings, QString& coverFileName);

  protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

  private slots:
    void on_DoubleClicked(const QModelIndex& index);
    void customContextMenuRequested(QPoint position);
    void generateColumnsMenu(void);

    void on_listViewWidget_sortIndicatorChanged(int logicalIndex, Qt::SortOrder sortOrder);
    void on_listViewWidget_sectionResized(int logicalIndex, int oldWidth, int newWidth);
    void on_listViewWidget_sectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    void on_listViewWidget_headerContextMenuRequested(QPoint position);
    
    void on_gridViewWidget_iconSizeChanged(const QSize& size);

    void on_ZoomIn(void);
    void on_ZoomOut(void);

    void on_RomBrowserThread_RomFound(QString file, CoreRomType type, CoreRomHeader header, CoreRomSettings settings, int index, int count);
    void on_RomBrowserThread_Finished(bool canceled);

    void on_Action_PlayGame(void);
    void on_Action_PlayGameWith(void);
    void on_Action_RefreshRomList(void);
    void on_Action_OpenRomDirectory(void);
    void on_Action_ChangeRomDirectory(void);
    void on_Action_RomInformation(void);
    void on_Action_EditGameSettings(void);
    void on_Action_EditCheats(void);
    void on_Action_ResetColumnSizes(void);
    void on_Action_SetCoverImage(void);
    void on_Action_RemoveCoverImage(void);

  signals:
    void PlayGame(QString);
    void PlayGameWith(CoreRomType, QString);
    void EditGameSettings(QString);
    void Cheats(QString);
    void ChangeRomDirectory(void);
    void RomInformation(QString);
};
} // namespace Widget
} // namespace UserInterface

#endif // ROMBROWSERWIDGET_HPP
