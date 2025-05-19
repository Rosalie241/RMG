/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
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
#include "RomBrowserSearchWidget.hpp"
#include "RomBrowserEmptyWidget.hpp"

#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QGridLayout>
#include <QListWidget>
#include <QHeaderView>
#include <QTableView>
#include <QLineEdit>
#include <QAction>
#include <QString>
#include <QList>
#include <QMenu>
#include <QPair>
#include <QMap>

// forward declaration of internal struct
struct RomBrowserModelData;

namespace UserInterface
{
namespace Widget
{
class RomBrowserWidget : public QWidget
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
    void SetToggleSearch(void);

    QMap<QString, CoreRomSettings> GetModelData(void);

  private:
    QStackedWidget* stackedWidget = nullptr;
    Widget::RomBrowserEmptyWidget*    emptyWidget    = nullptr;
    Widget::RomBrowserLoadingWidget*  loadingWidget  = nullptr;

    Widget::RomBrowserListViewWidget* listViewWidget = nullptr;
    QStandardItemModel* listViewModel                = nullptr;
    QSortFilterProxyModel* listViewProxyModel        = nullptr;
    Widget::RomBrowserGridViewWidget* gridViewWidget = nullptr;
    QStandardItemModel* gridViewModel                = nullptr;
    QSortFilterProxyModel* gridViewProxyModel        = nullptr;

    Widget::RomBrowserSearchWidget* searchWidget = nullptr;
    bool showSearchWidget = false;

    QWidget* currentViewWidget = nullptr;

    QElapsedTimer romSearcherTimer;
    Thread::RomSearcherThread* romSearcherThread = nullptr;
  
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
    QAction* action_EditGameInputSettings;
    QAction* action_EditCheats;
    QAction* action_ResetColumnSizes;
    QAction* action_SetCoverImage;
    QAction* action_RemoveCoverImage;

    QMenu*   menu_PlayGameWithDisk;
    QMenu*   menu_PlayGameWithSlot;
    QMenu*   menu_Columns;
    QAction* action_ColumnsMenuEntry;

    QString coversDirectory;

    QPair<QStandardItemModel*, QSortFilterProxyModel*> getCurrentModel(void);
    QAbstractItemView*  getCurrentModelView(void);
    bool getCurrentData(RomBrowserModelData& data);

    QString getCurrentRom(void);

    void addRomData(QString file, CoreRomType type, CoreRomHeader header, CoreRomSettings settings);

    QIcon getCurrentCover(QString file, CoreRomHeader header, CoreRomSettings settings, QString& coverFileName);

  protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

  private slots:
    void on_DoubleClicked(const QModelIndex& index);
    void customContextMenuRequested(QPoint position);
    void generateColumnsMenu(void);
    void generatePlayWithDiskMenu(void);
    void generateStateMenu(void);

    void on_searchWidget_SearchTextChanged(const QString& text);

    void on_listViewWidget_sortIndicatorChanged(int logicalIndex, Qt::SortOrder sortOrder);
    void on_listViewWidget_sectionResized(int logicalIndex, int oldWidth, int newWidth);
    void on_listViewWidget_sectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    void on_listViewWidget_headerContextMenuRequested(QPoint position);
  
    void on_gridViewWidget_iconSizeChanged(const QSize& size);

    void on_ZoomIn(void);
    void on_ZoomOut(void);

    void on_RomBrowserThread_RomsFound(QList<RomSearcherThreadData> data, int index, int count);
    void on_RomBrowserThread_Finished(bool canceled);

    void on_Action_PlayGame(void);
    void on_Action_PlayGameWith(void);
    void on_Menu_PlayGameWithDisk(QAction* action);
    void on_Action_PlayGameWithSlot(int);
    void on_Action_RefreshRomList(void);
    void on_Action_OpenRomDirectory(void);
    void on_Action_ChangeRomDirectory(void);
    void on_Action_RomInformation(void);
    void on_Action_EditGameSettings(void);
    void on_Action_EditGameInputSettings(void);
    void on_Action_EditCheats(void);
    void on_Action_ResetColumnSizes(void);
    void on_Action_SetCoverImage(void);
    void on_Action_RemoveCoverImage(void);

  signals:
    void PlayGame(QString file);
    void PlayGameWith(CoreRomType type, QString file);
    void PlayGameWithDisk(QString cartridge, QString disk);
    void PlayGameWithSlot(QString file, int slot);
    void EditGameSettings(QString file);
    void EditGameInputSettings(QString file);
    void Cheats(QString file);
    void ChangeRomDirectory(void);
    void RomInformation(QString file);

    void FileDropped(QDropEvent* event);
};
} // namespace Widget
} // namespace UserInterface

#endif // ROMBROWSERWIDGET_HPP
