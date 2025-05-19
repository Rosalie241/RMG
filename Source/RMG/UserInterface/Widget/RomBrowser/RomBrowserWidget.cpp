/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomBrowserWidget.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QGridLayout>
#include <QBoxLayout>
#include <QScrollBar>
#include <QPixmap>
#include <QLabel>
#include <vector>
#include <QList>
#include <QDir>

#include <RMG-Core/Directories.hpp>
#include <RMG-Core/SaveState.hpp>
#include <RMG-Core/Settings.hpp>
#include <RMG-Core/Plugins.hpp>

using namespace UserInterface::Widget;

//
// Internal Struct
//

struct RomBrowserModelData
{
    QString         file;
    CoreRomType     type;
    CoreRomHeader   header;
    CoreRomSettings settings;
    QString         coverFile;

    RomBrowserModelData() {}

    RomBrowserModelData(QString file, CoreRomType type, CoreRomHeader header, CoreRomSettings settings)
    {
        this->file     = file;
        this->type     = type;
        this->header   = header;
        this->settings = settings;
    }
};

Q_DECLARE_METATYPE(RomBrowserModelData);

//
// Exported Functions
// 

RomBrowserWidget::RomBrowserWidget(QWidget *parent) : QWidget(parent)
{
    // configure signal types
    qRegisterMetaType<CoreRomType>("CoreRomType");

    // configure stacked widget
    this->stackedWidget = new QStackedWidget(this);

    // configure search widget
    this->searchWidget = new Widget::RomBrowserSearchWidget(this);
    connect(this->searchWidget, &RomBrowserSearchWidget::SearchTextChanged, this, &RomBrowserWidget::on_searchWidget_SearchTextChanged);
    // configure layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(this->stackedWidget);
    layout->addWidget(this->searchWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    // configure rom searcher thread
    this->romSearcherThread = new Thread::RomSearcherThread(this);
    connect(this->romSearcherThread, &Thread::RomSearcherThread::RomsFound, this, &RomBrowserWidget::on_RomBrowserThread_RomsFound);
    connect(this->romSearcherThread, &Thread::RomSearcherThread::Finished, this, &RomBrowserWidget::on_RomBrowserThread_Finished);

    // configure empty widget
    this->emptyWidget = new Widget::RomBrowserEmptyWidget(this);
    this->stackedWidget->addWidget(this->emptyWidget);
    connect(this->emptyWidget, &RomBrowserEmptyWidget::SelectRomDirectory, this, &RomBrowserWidget::on_Action_ChangeRomDirectory);
    connect(this->emptyWidget, &RomBrowserEmptyWidget::Refresh, this, &RomBrowserWidget::on_Action_RefreshRomList);
    connect(this->emptyWidget, &RomBrowserEmptyWidget::FileDropped, this, &RomBrowserWidget::FileDropped);

    // configure loading widget
    this->loadingWidget = new Widget::RomBrowserLoadingWidget(this);
    connect(this->stackedWidget, &QStackedWidget::currentChanged, this->loadingWidget, &RomBrowserLoadingWidget::on_RomBrowserWidget_currentChanged);
    connect(this->loadingWidget, &RomBrowserLoadingWidget::FileDropped, this, &RomBrowserWidget::FileDropped);
    this->loadingWidget->SetWidgetIndex(this->stackedWidget->addWidget(this->loadingWidget));

    // configure list view widget
    this->listViewWidget = new Widget::RomBrowserListViewWidget(this);
    this->listViewModel  = new QStandardItemModel(this);
    this->listViewProxyModel = new QSortFilterProxyModel(this);
    this->listViewProxyModel->setSourceModel(this->listViewModel);
    this->listViewWidget->setModel(this->listViewProxyModel);
    this->listViewWidget->setFrameStyle(QFrame::NoFrame);
    this->listViewWidget->setItemDelegate(new NoFocusDelegate(this));
    this->listViewWidget->setWordWrap(false);
    this->listViewWidget->setShowGrid(false);
    this->listViewWidget->setSortingEnabled(true);
    this->listViewWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->listViewWidget->setSelectionBehavior(QTableView::SelectRows);
    this->listViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    this->listViewWidget->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    this->listViewWidget->verticalHeader()->hide();
    this->listViewWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->listViewWidget->horizontalHeader()->setSectionsMovable(true);
    this->listViewWidget->horizontalHeader()->setFirstSectionMovable(true);
    this->listViewWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    this->listViewWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    this->listViewWidget->horizontalHeader()->setSortIndicatorShown(false);
    this->listViewWidget->horizontalHeader()->setHighlightSections(false);
    this->listViewWidget->horizontalHeader()->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    this->stackedWidget->addWidget(this->listViewWidget);
    connect(this->listViewWidget, &QTableView::doubleClicked, this, &RomBrowserWidget::on_DoubleClicked);
    connect(this->listViewWidget->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &RomBrowserWidget::on_listViewWidget_sortIndicatorChanged);
    connect(this->listViewWidget->horizontalHeader(), &QHeaderView::sectionResized, this, &RomBrowserWidget::on_listViewWidget_sectionResized);
    connect(this->listViewWidget->horizontalHeader(), &QHeaderView::sectionMoved, this, &RomBrowserWidget::on_listViewWidget_sectionMoved);
    connect(this->listViewWidget->horizontalHeader(), &QHeaderView::customContextMenuRequested, this, &RomBrowserWidget::on_listViewWidget_headerContextMenuRequested);
    connect(this->listViewWidget, &Widget::RomBrowserListViewWidget::ZoomIn, this, &RomBrowserWidget::on_ZoomIn);
    connect(this->listViewWidget, &Widget::RomBrowserListViewWidget::ZoomOut, this, &RomBrowserWidget::on_ZoomOut);
    connect(this->listViewWidget, &Widget::RomBrowserListViewWidget::FileDropped, this, &RomBrowserWidget::FileDropped);

    // set up list view's columns
    QStringList labels;
    labels << "Name";
    labels << "Internal Name";
    labels << "MD5";
    labels << "Format";
    labels << "File Name";
    labels << "File Ext.";
    labels << "File Size";
    labels << "I.D.";
    labels << "Region";
    this->listViewModel->setColumnCount(labels.size());
    this->listViewModel->setHorizontalHeaderLabels(labels);

    // set full names of list view's columns
    this->columnNames << labels.at(0);
    this->columnNames << labels.at(1);
    this->columnNames << labels.at(2);
    this->columnNames << "Game Format";
    this->columnNames << labels.at(4);
    this->columnNames << "File Extension";
    this->columnNames << labels.at(6);
    this->columnNames << "Game I.D.";
    this->columnNames << "Game Region";

    // configure grid view widget
    this->gridViewWidget = new Widget::RomBrowserGridViewWidget(this);
    this->gridViewModel  = new QStandardItemModel(this);
    this->gridViewProxyModel = new QSortFilterProxyModel(this);
    this->gridViewProxyModel->setSourceModel(this->gridViewModel);
    this->gridViewWidget->setModel(this->gridViewProxyModel);
    this->gridViewWidget->setFlow(QListView::Flow::LeftToRight);
    this->gridViewWidget->setResizeMode(QListView::Adjust);
#ifndef DRAG_DROP
    this->gridViewWidget->setMovement(QListView::Static);
#endif // DRAG_DROP
    this->gridViewWidget->setUniformItemSizes(CoreSettingsGetBoolValue(SettingsID::RomBrowser_GridViewUniformItemSizes));
    this->gridViewWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->gridViewWidget->setViewMode(QListView::ViewMode::IconMode);
    this->gridViewWidget->setTextElideMode(Qt::TextElideMode::ElideNone);
    this->gridViewWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->gridViewWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->gridViewWidget->verticalScrollBar()->setSingleStep(15);
    this->gridViewWidget->setWordWrap(true);
    this->gridViewWidget->setFrameStyle(QFrame::NoFrame);
    int iconWidth = CoreSettingsGetIntValue(SettingsID::RomBrowser_GridViewIconWidth);
    int iconHeight = CoreSettingsGetIntValue(SettingsID::RomBrowser_GridViewIconHeight);
    this->gridViewWidget->setIconSize(QSize(iconWidth, iconHeight));
    this->stackedWidget->addWidget(this->gridViewWidget);
    connect(this->gridViewWidget, &QListView::doubleClicked, this, &RomBrowserWidget::on_DoubleClicked);
    connect(this->gridViewWidget, &QListView::iconSizeChanged, this, &RomBrowserWidget::on_gridViewWidget_iconSizeChanged);
    connect(this->gridViewWidget, &Widget::RomBrowserGridViewWidget::ZoomIn, this, &RomBrowserWidget::on_ZoomIn);
    connect(this->gridViewWidget, &Widget::RomBrowserGridViewWidget::ZoomOut, this, &RomBrowserWidget::on_ZoomOut);
    connect(this->gridViewWidget, &Widget::RomBrowserGridViewWidget::FileDropped, this, &RomBrowserWidget::FileDropped);

#ifdef DRAG_DROP
    // configure drag & drop
    this->setAcceptDrops(true);
#endif // DRAG_DROP

    // configure context menu policy
    this->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(this, &QStackedWidget::customContextMenuRequested, this, &RomBrowserWidget::customContextMenuRequested);

    // define context menu and its contents
    this->contextMenu = new QMenu(this);
    this->action_PlayGame = new QAction(this);
    this->action_PlayGameWith = new QAction(this);
    this->menu_PlayGameWithDisk = new QMenu(this->contextMenu);
    this->menu_PlayGameWithSlot = new QMenu(this->contextMenu);
    this->action_RefreshRomList = new QAction(this);
    this->action_OpenRomDirectory = new QAction(this);
    this->action_ChangeRomDirectory = new QAction(this);
    this->action_RomInformation = new QAction(this);
    this->action_EditGameSettings = new QAction(this);
    this->action_EditGameInputSettings = new QAction(this);
    this->action_EditCheats = new QAction(this);
    this->action_ResetColumnSizes = new QAction(this);
    this->action_SetCoverImage = new QAction(this);
    this->action_RemoveCoverImage = new QAction(this);

    // define columns menu and its contents
    this->menu_Columns = new QMenu(this->contextMenu);
    this->action_ColumnsMenuEntry = new QAction(this);

    // configure context menu contents
    this->action_PlayGame->setText("Play Game");
    this->action_PlayGameWith->setText("Play Game with Disk");
    this->menu_PlayGameWithDisk->menuAction()->setText("Play Game with Disk");
    this->menu_PlayGameWithSlot->menuAction()->setText("Play Game with State");
    this->action_RefreshRomList->setText("Refresh ROM List");
    this->action_OpenRomDirectory->setText("Open ROM Directory");
    this->action_ChangeRomDirectory->setText("Change ROM Directory...");
    this->action_RomInformation->setText("ROM Information");
    this->action_EditGameSettings->setText("Edit Game Settings");
    this->action_EditGameInputSettings->setText("Edit Game Input Settings");
    this->action_EditCheats->setText("Edit Cheats");
    this->action_ResetColumnSizes->setText("Reset Column Sizes");
    this->menu_Columns->menuAction()->setText("Show/Hide Columns");
    this->action_SetCoverImage->setText("Set Cover Image...");
    this->action_RemoveCoverImage->setText("Remove Cover Image");
    connect(this->action_PlayGame, &QAction::triggered, this, &RomBrowserWidget::on_Action_PlayGame);
    connect(this->action_PlayGameWith, &QAction::triggered, this, &RomBrowserWidget::on_Action_PlayGameWith);
    connect(this->menu_PlayGameWithDisk, &QMenu::triggered, this, &RomBrowserWidget::on_Menu_PlayGameWithDisk);
    connect(this->action_RefreshRomList, &QAction::triggered, this, &RomBrowserWidget::on_Action_RefreshRomList);
    connect(this->action_OpenRomDirectory, &QAction::triggered, this, &RomBrowserWidget::on_Action_OpenRomDirectory);
    connect(this->action_ChangeRomDirectory, &QAction::triggered, this, &RomBrowserWidget::on_Action_ChangeRomDirectory);
    connect(this->action_RomInformation, &QAction::triggered, this, &RomBrowserWidget::on_Action_RomInformation);
    connect(this->action_EditGameSettings, &QAction::triggered, this, &RomBrowserWidget::on_Action_EditGameSettings);
    connect(this->action_EditGameInputSettings, &QAction::triggered, this, &RomBrowserWidget::on_Action_EditGameInputSettings);
    connect(this->action_EditCheats, &QAction::triggered, this, &RomBrowserWidget::on_Action_EditCheats);
    connect(this->action_ResetColumnSizes, &QAction::triggered, this, &RomBrowserWidget::on_Action_ResetColumnSizes);
    connect(this->action_SetCoverImage, &QAction::triggered, this, &RomBrowserWidget::on_Action_SetCoverImage);
    connect(this->action_RemoveCoverImage, &QAction::triggered, this, &RomBrowserWidget::on_Action_RemoveCoverImage);

    // configure context menu
    this->contextMenu->addAction(this->action_PlayGame);
    this->contextMenu->addAction(this->action_PlayGameWith);
    this->contextMenu->addMenu(this->menu_PlayGameWithDisk);
    this->contextMenu->addMenu(this->menu_PlayGameWithSlot);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_RefreshRomList);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_OpenRomDirectory);
    this->contextMenu->addAction(this->action_ChangeRomDirectory);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_RomInformation);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_EditGameSettings);
    this->contextMenu->addAction(this->action_EditGameInputSettings);
    this->contextMenu->addAction(this->action_EditCheats);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_ResetColumnSizes);
    this->contextMenu->addMenu(this->menu_Columns);
    this->contextMenu->addAction(this->action_SetCoverImage);
    this->contextMenu->addAction(this->action_RemoveCoverImage);

    // configure current view widget
    int currentView = CoreSettingsGetIntValue(SettingsID::RomBrowser_ViewMode);
    if (currentView == 0)
    {
        this->currentViewWidget = this->listViewWidget;
    }
    else
    {
        this->currentViewWidget = this->gridViewWidget;
    }
}

RomBrowserWidget::~RomBrowserWidget()
{
}

void RomBrowserWidget::RefreshRomList(void)
{
    this->listViewModel->removeRows(0, this->listViewModel->rowCount());
    this->gridViewModel->removeRows(0, this->gridViewModel->rowCount());

    this->menu_PlayGameWithDisk->clear();

    this->coversDirectory = QString::fromStdString(CoreGetUserDataDirectory().string());
    this->coversDirectory += CORE_DIR_SEPERATOR_STR;
    this->coversDirectory += "Covers";

    this->listViewSortSection = CoreSettingsGetIntValue(SettingsID::RomBrowser_ListViewSortSection);
    this->listViewSortOrder   = CoreSettingsGetIntValue(SettingsID::RomBrowser_ListViewSortOrder);

    QString directory = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::RomBrowser_Directory));
    if (directory.isEmpty())
    {
        this->stackedWidget->setCurrentWidget(this->emptyWidget);
        return;
    }

    this->stackedWidget->setCurrentWidget(this->loadingWidget);

    this->showSearchLineEdit = this->searchWidget->isVisible();
    this->searchWidget->hide();

    this->romSearcherTimer.start();

    this->romSearcherThread->SetMaximumFiles(CoreSettingsGetIntValue(SettingsID::RomBrowser_MaxItems));
    this->romSearcherThread->SetRecursive(CoreSettingsGetBoolValue(SettingsID::RomBrowser_Recursive));
    this->romSearcherThread->SetDirectory(directory);
    this->romSearcherThread->start();
}

bool RomBrowserWidget::IsRefreshingRomList(void)
{
    return this->romSearcherThread->isRunning();
}

void RomBrowserWidget::StopRefreshRomList(void)
{
    return this->romSearcherThread->Stop();
}

void RomBrowserWidget::ShowList(void)
{
    this->currentViewWidget = this->listViewWidget;

    // only change widget now when we're not refreshing
    if (!this->IsRefreshingRomList() &&
        this->stackedWidget->currentWidget() != this->emptyWidget)
    {
        this->stackedWidget->setCurrentWidget(this->listViewWidget);
    }
}

void RomBrowserWidget::ShowGrid(void)
{
    this->currentViewWidget = this->gridViewWidget;

    // only change widget now when we're not refreshing
    if (!this->IsRefreshingRomList() &&
        this->stackedWidget->currentWidget() != this->emptyWidget)
    {
        this->stackedWidget->setCurrentWidget(this->gridViewWidget);
    }
}

void RomBrowserWidget::SetGridViewUniformSizes(bool value)
{
    // refresh ROM list when we're currently in the grid view
    // and we're not refreshing already
    if (this->stackedWidget->currentWidget() == this->gridViewWidget &&
        !this->IsRefreshingRomList())
    {
        if (this->gridViewWidget->uniformItemSizes() != value)
        {
            this->gridViewWidget->setUniformItemSizes(value);
            this->RefreshRomList();
            return;
        }
    }

    this->gridViewWidget->setUniformItemSizes(value);
}

void RomBrowserWidget::SetToggleSearch(void)
{
    const bool show = !this->searchWidget->isVisible();

    // do nothing when not in a valid view
    QWidget* currentWidget = this->stackedWidget->currentWidget();
    if (currentWidget == this->loadingWidget ||
        currentWidget == this->emptyWidget)
    {
        return;
    }

    this->searchWidget->setVisible(show);

    if (show)
    {
        // set focus when it's being shown
        this->searchWidget->FocusSearchText();
    }
    else
    {
        // reset search when hidden
        this->searchWidget->ClearSearchTerm();
    }
}

QMap<QString, CoreRomSettings> RomBrowserWidget::GetModelData(void)
{
    QMap<QString, CoreRomSettings> data;
    QPair<QStandardItemModel*, QSortFilterProxyModel*> model = this->getCurrentModel();
    RomBrowserModelData modelData;

    if (model.first == nullptr)
    {
        return data;
    }

    for (int i = 0; i < model.first->rowCount(); i++)
    {
        modelData = model.first->item(i)->data().value<RomBrowserModelData>();
        // only add cartridges, 64dd disks aren't supported
        if (modelData.type == CoreRomType::Cartridge)
        {
            data.insert(modelData.file, modelData.settings);
        }
    }

    return data;
}

QPair<QStandardItemModel*, QSortFilterProxyModel*> RomBrowserWidget::getCurrentModel(void)
{
    QWidget* currentWidget = this->stackedWidget->currentWidget();
    if (currentWidget == this->loadingWidget)
    {
        currentWidget = this->currentViewWidget;
    }

    if (currentWidget == this->listViewWidget)
    {
        return qMakePair(this->listViewModel, this->listViewProxyModel);
    }
    else if (currentWidget == this->gridViewWidget)
    {
        return qMakePair(this->gridViewModel, this->gridViewProxyModel);
    }

    return qMakePair(nullptr, nullptr);
}

QAbstractItemView* RomBrowserWidget::getCurrentModelView(void)
{
    QWidget* currentWidget = this->stackedWidget->currentWidget();
    if (currentWidget == this->loadingWidget)
    {
        currentWidget = this->currentViewWidget;
    }

    if (currentWidget == this->listViewWidget)
    {
        return this->listViewWidget;
    }
    else if (currentWidget == this->gridViewWidget)
    {
        return this->gridViewWidget;
    }

    return nullptr;
}

bool RomBrowserWidget::getCurrentData(RomBrowserModelData& data)
{
    QPair<QStandardItemModel*, QSortFilterProxyModel*> model = this->getCurrentModel();
    QAbstractItemView* view = this->getCurrentModelView();

    if (model.first == nullptr || model.second == nullptr || view == nullptr)
    {
        return false;
    }

    QModelIndex    index = model.second->mapToSource(view->currentIndex());
    QStandardItem* item  = model.first->item(index.row(), 0);

    if (item == nullptr)
    {
        return false;
    }

    data = item->data().value<RomBrowserModelData>();
    return true;
}


QString RomBrowserWidget::getCurrentRom(void)
{
    RomBrowserModelData data;

    if (!this->getCurrentData(data))
    {
        return "";
    }

    return data.file;
}

void RomBrowserWidget::addRomData(QString file, CoreRomType type, CoreRomHeader header, CoreRomSettings settings)
{
    QString name;
    QString gameFormat;
    float fileSize;
    QString fileSizeString;
    QString coverFile;
    QIcon   coverIcon;
    QVariant itemData;
    RomBrowserModelData modelData;

    // create item data
    modelData = RomBrowserModelData(file, type, header, settings);

    // generate name to use in UI
    name = QString::fromStdString(settings.GoodName);
    if (name.endsWith("(unknown rom)") ||
        name.endsWith("(unknown disk)"))
    {
        name = QFileInfo(file).fileName();
    }

    // generate game format to use in UI
    if (type == CoreRomType::Disk)
    {
        gameFormat = "Disk";
    }
    else
    {
        gameFormat = "Cartridge";
    }

    // generate file size to use in UI
    fileSize = QFileInfo(file).size()/1048576.0;
    fileSizeString = (QString::number(fileSize, 'f', 2)).append(" MB");
    if (fileSizeString.size() == 7)
    {
        fileSizeString = fileSizeString.prepend("  ");
    }

    // retrieve cover image
    coverIcon = this->getCurrentCover(file, header, settings, coverFile);
    modelData.coverFile = coverFile;

    // create item data
    itemData = QVariant::fromValue<RomBrowserModelData>(modelData);

    QList<QStandardItem*> listViewRow;
    // goodname
    QStandardItem* listViewItem1 = new QStandardItem();
    listViewItem1->setText(name);
    listViewItem1->setData(itemData);
    listViewRow.append(listViewItem1);
    // internal name
    QStandardItem* listViewItem2 = new QStandardItem();
    listViewItem2->setText(QString::fromStdString(header.Name));
    listViewRow.append(listViewItem2);
    // MD5
    QStandardItem* listViewItem3 = new QStandardItem();
    listViewItem3->setText(QString::fromStdString(settings.MD5));
    listViewRow.append(listViewItem3);
    // game format
    QStandardItem* listViewItem4 = new QStandardItem();
    listViewItem4->setText(gameFormat);
    listViewRow.append(listViewItem4);
    // file name
    QStandardItem* listViewItem5 = new QStandardItem();
    listViewItem5->setText(QFileInfo(file).completeBaseName());
    listViewRow.append(listViewItem5);
    // file extension
    QStandardItem* listViewItem6 = new QStandardItem();
    listViewItem6->setText(((QFileInfo(file).suffix()).prepend(".")).toUpper());
    listViewRow.append(listViewItem6);
    // file size
    QStandardItem* listViewItem7 = new QStandardItem();
    listViewItem7->setText(fileSizeString);
    listViewRow.append(listViewItem7);
    // game i.d.
    QStandardItem* listViewItem8 = new QStandardItem();
    listViewItem8->setText(QString::fromStdString(header.GameID));
    listViewRow.append(listViewItem8);
    // region
    QStandardItem* listViewItem9 = new QStandardItem();
    listViewItem9->setText(QString::fromStdString(header.Region));
    listViewRow.append(listViewItem9);
    this->listViewModel->appendRow(listViewRow);

    QStandardItem* gridViewItem = new QStandardItem();
    gridViewItem->setIcon(coverIcon);
    gridViewItem->setText(name);
    gridViewItem->setData(itemData);
    this->gridViewModel->appendRow(gridViewItem);
}

QIcon RomBrowserWidget::getCurrentCover(QString file, CoreRomHeader header, CoreRomSettings settings, QString& coverFileName)
{
    QPixmap pixmap;

    // construct basename of file,
    // by retrieving the last index of '.'
    // and removing all characters from that index
    // until the end of the string
    QString baseName         = QFileInfo(file).fileName();
    qsizetype lastIndexOfDot = baseName.lastIndexOf(".");
    if (lastIndexOfDot != -1)
    { // only remove when index was found
        baseName.remove(lastIndexOfDot, baseName.size() - lastIndexOfDot);
    }

    // try to load cover using
    // 1) basename of file
    // 2) MD5
    // 3) good name
    // 4) internal name
    bool foundCover = false;
    for (QString name : { 
        baseName,
        QString::fromStdString(settings.MD5), 
        QString::fromStdString(settings.GoodName), 
        QString::fromStdString(header.Name) })
    {
        // fixup file name
        QString fixedName = name;
        for (const QChar c : QString(":<>\"/\\|?*"))
        {
            fixedName.replace(c, "_");
        }

        // skip empty names,
        // this can i.e happen
        // when ROMs don't have 
        // an internal ROM name
        if (fixedName.isEmpty())
        {
            continue;
        }

        // we support jpg & png as file extensions
        for (QString ext : { ".png", ".jpg", ".jpeg" })
        {
            QString coverPath = this->coversDirectory;
            coverPath += CORE_DIR_SEPERATOR_STR;
            coverPath += fixedName;
            coverPath += ext;

            if (QFile::exists(coverPath) && 
                pixmap.load(coverPath))
            {
                coverFileName = coverPath;
                foundCover = true;
                break;
            }
        }

        if (foundCover)
        {
            break;
        }
    }
    // fallback
    if (!foundCover)
    {
        pixmap.load(":Resource/CoverFallback.png");
    }

    return QIcon(pixmap);
}

void RomBrowserWidget::timerEvent(QTimerEvent* event)
{
    this->killTimer(event->timerId());
    this->stackedWidget->setCurrentWidget(this->currentViewWidget);
    this->searchWidget->setVisible(this->showSearchLineEdit);
}

void RomBrowserWidget::on_DoubleClicked(const QModelIndex& index)
{
    emit this->PlayGame(this->getCurrentRom());
}

void RomBrowserWidget::customContextMenuRequested(QPoint position)
{
    QAbstractItemView* view = this->getCurrentModelView();
    if (view == nullptr)
    {
        return;
    }

    RomBrowserModelData data;
    bool hasSelection = view->selectionModel()->hasSelection();
    bool showPlayWithDiskMenu;

    if (!this->getCurrentData(data))
    {
        return;
    }

    showPlayWithDiskMenu = data.type == CoreRomType::Cartridge && !this->menu_PlayGameWithDisk->isEmpty();

    this->action_PlayGame->setEnabled(hasSelection);
    this->action_PlayGameWith->setEnabled(hasSelection);
    this->action_PlayGameWith->setVisible(!showPlayWithDiskMenu);
    this->menu_PlayGameWithDisk->menuAction()->setVisible(showPlayWithDiskMenu);
    this->menu_PlayGameWithSlot->menuAction()->setEnabled(hasSelection);
    this->action_RomInformation->setEnabled(hasSelection);
    this->action_EditGameSettings->setEnabled(hasSelection);
    this->action_EditGameInputSettings->setEnabled(hasSelection && CorePluginsHasROMConfig(CorePluginType::Input));
    this->action_EditCheats->setEnabled(hasSelection);
    this->action_ResetColumnSizes->setVisible(view == this->listViewWidget);
    this->menu_Columns->menuAction()->setVisible(view == this->listViewWidget);
    this->action_SetCoverImage->setEnabled(hasSelection);
    this->action_SetCoverImage->setVisible(view == this->gridViewWidget);
    this->action_RemoveCoverImage->setEnabled(hasSelection && !data.coverFile.isEmpty());
    this->action_RemoveCoverImage->setVisible(view == this->gridViewWidget);

    if (hasSelection)
    {
        this->generateStateMenu();
    }

    if (hasSelection && data.type == CoreRomType::Disk)
    { // disk selected
        this->action_PlayGameWith->setText("Play Game with Cartridge...");
    }
    else
    { // cartridge selected
        this->action_PlayGameWith->setText("Play Game with Disk...");
    }

    if (view == this->listViewWidget)
    { // list view
        this->generateColumnsMenu();
    }

    if (view == this->gridViewWidget)
    { // grid view
        if (data.coverFile.isEmpty())
        {
            this->action_SetCoverImage->setText("Set Cover Image...");
        }
        else
        {
            this->action_SetCoverImage->setText("Change Cover Image...");
        }
    }

    this->contextMenu->popup(this->mapToGlobal(position));
}

void RomBrowserWidget::generateColumnsMenu(void)
{
    this->menu_Columns->clear();

    for (int i = 0; i < this->listViewModel->columnCount(); i++)
    {
        int column = this->listViewWidget->horizontalHeader()->logicalIndex(i);

        this->action_ColumnsMenuEntry = menu_Columns->addAction(this->columnNames.at(column));
        this->action_ColumnsMenuEntry->setCheckable(true);
        this->action_ColumnsMenuEntry->setChecked(!this->listViewWidget->horizontalHeader()->isSectionHidden(column));
        connect(this->action_ColumnsMenuEntry, &QAction::toggled, [this, column](bool checked)
        {
            this->listViewWidget->horizontalHeader()->setSectionHidden(column, !checked);
        });
    }
}

void RomBrowserWidget::generatePlayWithDiskMenu(void)
{
    QAction* playGameWithAction;
    QPair<QStandardItemModel*, QSortFilterProxyModel*> model = this->getCurrentModel();
    RomBrowserModelData modelData;
    int count = 0;

    this->menu_PlayGameWithDisk->clear();

    if (model.first == nullptr)
    {
        return;
    }

    for (int i = 0; i < model.first->rowCount(); i++)
    {
        modelData = model.first->item(i)->data().value<RomBrowserModelData>();
        if (modelData.type == CoreRomType::Disk)
        {
            if (count == 0)
            { // only add 'Browse...' action when there are disks
                this->menu_PlayGameWithDisk->addAction("Browse...");
                this->menu_PlayGameWithDisk->addSeparator();
            }

            playGameWithAction = new QAction(this);
            playGameWithAction->setText(model.first->item(i)->text());
            playGameWithAction->setData(model.first->item(i)->data());
            this->menu_PlayGameWithDisk->addAction(playGameWithAction);

            // only add 10 disks to menu,
            // to prevent an overwhelming amount
            // of menu items
            if (count++ >= 10)
            {
                return;
            }
        }
    }
}

void RomBrowserWidget::generateStateMenu(void)
{
    this->menu_PlayGameWithSlot->clear();

    RomBrowserModelData data;

    if (!this->getCurrentData(data))
    {
        return;
    }

    std::filesystem::path saveStatePath;
    QString saveStateSlotText;

    for (int i = 0; i < 10; i++)
    {
        if (!CoreGetSaveStatePath(data.header, data.settings, i, saveStatePath))
        {
            continue;
        }

        QFileInfo saveStateFileInfo(QString::fromStdWString(saveStatePath.wstring()));
        if (!saveStatePath.empty() && saveStateFileInfo.exists())
        {
            saveStateSlotText = "Slot " + QString::number(i) + " - ";
            saveStateSlotText += saveStateFileInfo.lastModified().toString("dd/MM/yyyy hh:mm");

            QAction* slotAction = this->menu_PlayGameWithSlot->addAction(saveStateSlotText);
            connect(slotAction, &QAction::triggered, [=, this]()
            {
                QString slotText = slotAction->text().split(" ").at(1);
                // sometimes the text can contain a '&'
                // which will make the toInt() function return 0
                // so strip it out
                slotText.remove('&');
                this->on_Action_PlayGameWithSlot(slotText.toInt());
            });
        }
    }

    this->menu_PlayGameWithSlot->setDisabled(this->menu_PlayGameWithSlot->isEmpty());
}

void RomBrowserWidget::on_searchWidget_SearchTextChanged(const QString& text)
{
    // TODO: I should really make a custom data
    // model at this point to save memory & CPU cycles...
    this->listViewProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    this->listViewProxyModel->setFilterWildcard(text);
    this->listViewProxyModel->setFilterKeyColumn(0);

    this->gridViewProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    this->gridViewProxyModel->setFilterWildcard(text);
    this->gridViewProxyModel->setFilterKeyColumn(0);
}

void RomBrowserWidget::on_listViewWidget_sortIndicatorChanged(int logicalIndex, Qt::SortOrder sortOrder)
{
    CoreSettingsSetValue(SettingsID::RomBrowser_ListViewSortSection, logicalIndex);
    CoreSettingsSetValue(SettingsID::RomBrowser_ListViewSortOrder, (int)sortOrder);
}

void RomBrowserWidget::on_listViewWidget_sectionResized(int logicalIndex, int oldWidth, int newWidth)
{
    std::vector<int> columnSizes = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnSizes);

    // this only happens when the ROM browser is
    // refreshing and the user exits
    if (columnSizes.empty())
    {
        return;
    }

    if (!this->listViewWidget->horizontalHeader()->stretchLastSection())
    {
        columnSizes.at(logicalIndex) = newWidth;
        CoreSettingsSetValue(SettingsID::RomBrowser_ColumnSizes, columnSizes);
    }
    else
    {
        std::vector<int> columnVisibility = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnVisibility);

        if (newWidth == 0)
        {
            columnVisibility.at(logicalIndex) = 0;
            CoreSettingsSetValue(SettingsID::RomBrowser_ColumnVisibility, columnVisibility);
        }
        else
        {
            columnVisibility.at(logicalIndex) = 1;
            CoreSettingsSetValue(SettingsID::RomBrowser_ColumnVisibility, columnVisibility);

            int lastVisibleColumn = -1;
            for (int i = 0; i < this->listViewModel->columnCount(); i++)
            {
                int column = this->listViewWidget->horizontalHeader()->logicalIndex(i);
                if (!this->listViewWidget->horizontalHeader()->isSectionHidden(column))
                {
                    lastVisibleColumn = column;
                }
            }

            if (logicalIndex != lastVisibleColumn)
            {
                columnSizes.at(logicalIndex) = newWidth;
                CoreSettingsSetValue(SettingsID::RomBrowser_ColumnSizes, columnSizes);
            }
        }
    }
}

void RomBrowserWidget::on_listViewWidget_sectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex)
{
    std::vector<int> columnOrder = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnOrder);

    for (size_t i = 0; i < columnOrder.size(); i++)
    {
        columnOrder.at(i) = this->listViewWidget->horizontalHeader()->visualIndex(i);
    }

    CoreSettingsSetValue(SettingsID::RomBrowser_ColumnOrder, columnOrder);
}

void RomBrowserWidget::on_listViewWidget_headerContextMenuRequested(QPoint position)
{
    this->generateColumnsMenu();
    this->menu_Columns->popup(this->mapToGlobal(position));
}

void RomBrowserWidget::on_gridViewWidget_iconSizeChanged(const QSize& size)
{
    CoreSettingsSetValue(SettingsID::RomBrowser_GridViewIconWidth, size.width());
    CoreSettingsSetValue(SettingsID::RomBrowser_GridViewIconHeight, size.height());
}

void RomBrowserWidget::on_ZoomIn(void)
{
    QAbstractItemView* view = this->getCurrentModelView();
    if (view == nullptr)
    {
        return;
    }

    view->setIconSize(view->iconSize() + QSize(10, 10));
}

void RomBrowserWidget::on_ZoomOut(void)
{
    QAbstractItemView* view = this->getCurrentModelView();
    if (view == nullptr)
    {
        return;
    }

    view->setIconSize(view->iconSize() - QSize(10, 10));
}

void RomBrowserWidget::on_RomBrowserThread_RomsFound(QList<RomSearcherThreadData> data, int index, int count)
{
    // add every item to our dataset
    for (qsizetype i = 0; i < data.size(); i++)
    {
        this->addRomData(data[i].File, data[i].Type, data[i].Header, data[i].Settings);
    }

    // update loading widget
    this->loadingWidget->SetCurrentRomIndex(index, count);
}

void RomBrowserWidget::on_RomBrowserThread_Finished(bool canceled)
{
    // sort data
    this->listViewProxyModel->sort(this->listViewSortSection, (Qt::SortOrder)this->listViewSortOrder);
    this->gridViewProxyModel->sort(0, Qt::SortOrder::AscendingOrder);

    // retrieve column settings
    std::vector<int> columnSizes = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnSizes);
    std::vector<int> columnOrder = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnOrder);
    std::vector<int> columnVisibility = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnVisibility);

    // temporarily disable stretching last column in list view
    this->listViewWidget->horizontalHeader()->setStretchLastSection(false);

    // reset column sizes setting in config file if number of values is incorrect
    if (!columnSizes.empty() && 
        columnSizes.size() != this->listViewModel->columnCount())
    {
        columnSizes.clear();
        columnSizes.resize(this->listViewModel->columnCount(), -1);
        CoreSettingsSetValue(SettingsID::RomBrowser_ColumnSizes, columnSizes);
    }

    // update list view's column sizes when
    // we have any rows in our list view
    if (this->listViewModel->rowCount() != 0)
    {
        for (size_t i = 0; i < columnSizes.size(); i++)
        {
            // set column widths to values specified in config file (or resize to content if not already specified)
            if (columnSizes.at(i) == -1)
            {
                this->listViewWidget->resizeColumnToContents(i);
            }
            else
            {
                this->listViewWidget->setColumnWidth(i, columnSizes.at(i));
            }
        }
    }

    // enable stretching last column in list view
    this->listViewWidget->horizontalHeader()->setStretchLastSection(true);

    // reset column order setting in config file if number of values is incorrect
    if (!columnOrder.empty() &&
        columnOrder.size() != this->listViewModel->columnCount())
    {
        columnOrder.clear();
        for (int i = 0; i < this->listViewModel->columnCount(); i++)
        {
            columnOrder.push_back(i);
        }
        CoreSettingsSetValue(SettingsID::RomBrowser_ColumnOrder, columnOrder);
    }

    // update list view's column order
    for (size_t i = 0; i < columnOrder.size(); i++)
    {
        this->listViewWidget->horizontalHeader()->moveSection(this->listViewWidget->horizontalHeader()->visualIndex(i), columnOrder.at(i));
    }

    // reset column visibility setting in config file if number of values is incorrect
    if (!columnVisibility.empty() &&
        columnVisibility.size() != this->listViewModel->columnCount())
    {
        columnVisibility.clear();
        columnVisibility.resize(this->listViewModel->columnCount(), 0);
        for (int i = 0; i < 3; i++)
        {
            columnVisibility.at(i) = 1;
        }
        CoreSettingsSetValue(SettingsID::RomBrowser_ColumnVisibility, columnVisibility);
    }

    // update list view's column visibilities
    for (size_t i = 0; i < columnVisibility.size(); i++)
    {
        if (columnVisibility.at(i) == 0)
        {
            this->listViewWidget->horizontalHeader()->setSectionHidden(i, true);
        }
    }

    // generate 'Play Game with Disk' menu here
    // instead of when a context menu has been requested,
    // this should save a lot of otherwise wasted CPU cycles
    this->generatePlayWithDiskMenu();

    // when canceled, we shouldn't switch to the grid/list view
    // because that can cause some flicker, so just return here
    // and don't do anything because a refresh will be triggered
    // later on anyways
    if (canceled)
    {
        return;
    }

    if (this->listViewModel->rowCount() == 0)
    {
        this->stackedWidget->setCurrentWidget(this->emptyWidget);
        return;
    }

    // prevent flicker by forcing the loading screen
    // to be shown at least 300ms
    qint64 elapsedTime = this->romSearcherTimer.elapsed();
    if (elapsedTime < 300)
    {
        this->startTimer(300 - elapsedTime);
        return;
    }

    this->stackedWidget->setCurrentWidget(this->currentViewWidget);
    this->searchWidget->setVisible(this->showSearchLineEdit);
}

void RomBrowserWidget::on_Action_PlayGame(void)
{
    emit this->PlayGame(this->getCurrentRom());
}

void RomBrowserWidget::on_Action_PlayGameWith(void)
{
    RomBrowserModelData data;

    if (!this->getCurrentData(data))
    {
        return;
    }

    emit this->PlayGameWith(data.type, data.file);
}

void RomBrowserWidget::on_Menu_PlayGameWithDisk(QAction* action)
{
    RomBrowserModelData cartridgeData;
    RomBrowserModelData diskData;
    bool hasDiskData = !action->data().isNull();

    if (!this->getCurrentData(cartridgeData))
    {
        return;
    }

    if (hasDiskData)
    {
        diskData = action->data().value<RomBrowserModelData>();
        emit this->PlayGameWithDisk(cartridgeData.file, diskData.file);
    }
    else
    {
        emit this->PlayGameWith(CoreRomType::Cartridge, cartridgeData.file);
    }
}

void RomBrowserWidget::on_Action_PlayGameWithSlot(int slot)
{
    RomBrowserModelData data;

    if (!this->getCurrentData(data))
    {
        return;
    }

    emit this->PlayGameWithSlot(data.file, slot);
}

void RomBrowserWidget::on_Action_RefreshRomList(void)
{
    this->RefreshRomList();
}

void RomBrowserWidget::on_Action_OpenRomDirectory(void)
{
    QString directory = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::RomBrowser_Directory));

    if (directory.isEmpty())
    {
        return;
    }

    QDesktopServices::openUrl(QUrl("file:///" + directory));
}   

void RomBrowserWidget::on_Action_ChangeRomDirectory(void)
{
    emit this->ChangeRomDirectory();
}

void RomBrowserWidget::on_Action_RomInformation(void)
{
    emit this->RomInformation(this->getCurrentRom());
}

void RomBrowserWidget::on_Action_EditGameSettings(void)
{
    emit this->EditGameSettings(this->getCurrentRom());
}

void RomBrowserWidget::on_Action_EditGameInputSettings(void)
{
    emit this->EditGameInputSettings(this->getCurrentRom());
}

void RomBrowserWidget::on_Action_EditCheats(void)
{
    emit this->Cheats(this->getCurrentRom());
}

void RomBrowserWidget::on_Action_ResetColumnSizes(void)
{
    std::vector<int> columnVisibility = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnVisibility);
    this->listViewWidget->horizontalHeader()->setStretchLastSection(false);

    for (int i = 0; i < this->listViewModel->columnCount(); i++)
    {
        this->listViewWidget->horizontalHeader()->setSectionHidden(i, false);
    }

    this->listViewWidget->resizeColumnsToContents();
    this->listViewWidget->horizontalHeader()->setStretchLastSection(true);

    for (size_t i = 0; i < columnVisibility.size(); i++)
    {
        if (columnVisibility.at(i) == 0)
        {
            this->listViewWidget->horizontalHeader()->setSectionHidden(i, true);
        }
    }
}

void RomBrowserWidget::on_Action_SetCoverImage(void)
{
    QString sourceFile;
    QFileInfo sourceFileInfo;
    QString coverFile;

    QPair<QStandardItemModel*, QSortFilterProxyModel*> model = this->getCurrentModel();
    QAbstractItemView* view  = this->getCurrentModelView();
    if (model.first == nullptr || model.second == nullptr || view == nullptr)
    {
        return;
    }

    if (this->coversDirectory.isEmpty())
    {
        return;
    }

    sourceFile = QFileDialog::getOpenFileName(this, tr("Open Cover Image"), "", tr("Cover Image (*.png *.jpeg *.jpg)"));
    if (sourceFile.isEmpty())
    {
        return;
    }

    // retrieve file info
    sourceFileInfo = QFileInfo(sourceFile);

    QModelIndex         index = model.second->mapToSource(view->currentIndex());
    QStandardItem*      item  = model.first->item(index.row(), index.column());
    RomBrowserModelData data  = model.first->itemData(index).last().value<RomBrowserModelData>();

    // construct new file name (for the cover)
    QString newFileName = this->coversDirectory;
    newFileName += CORE_DIR_SEPERATOR_STR;
    newFileName += QString::fromStdString(data.settings.MD5);
    newFileName += ".";
    newFileName += sourceFileInfo.suffix();

    // create covers directory when needed
    if (!QDir().exists(this->coversDirectory))
    {
        QDir().mkdir(this->coversDirectory);
    }

    // remove old cover when
    // cover file exists
    // and contains the MD5
    if (!data.coverFile.isEmpty() && 
        QFile::exists(data.coverFile) &&
        data.coverFile.contains(QString::fromStdString(data.settings.MD5)))
    {
        QFile::remove(data.coverFile);
    }

    // copy new one
    QFile::copy(sourceFile, newFileName);

    // update item
    item->setIcon(this->getCurrentCover(data.file, data.header, data.settings, coverFile));
    data.coverFile = coverFile;
    item->setData(QVariant::fromValue<RomBrowserModelData>(data));
}

void RomBrowserWidget::on_Action_RemoveCoverImage(void)
{
    QPair<QStandardItemModel*, QSortFilterProxyModel*> model = this->getCurrentModel();
    QAbstractItemView*  view  = this->getCurrentModelView();
    if (model.first == nullptr || model.second == nullptr || view == nullptr)
    {
        return;
    }

    QModelIndex         index = model.second->mapToSource(view->currentIndex());
    QStandardItem*      item  = model.first->item(index.row(), index.column());
    RomBrowserModelData data  = model.first->itemData(index).last().value<RomBrowserModelData>();
    QString coverFile;

    if (!data.coverFile.isEmpty() && QFile::exists(data.coverFile))
    {
        QFile::remove(data.coverFile);
    }

    // update item
    item->setIcon(this->getCurrentCover(data.file, data.header, data.settings, coverFile));
    data.coverFile = coverFile;
    item->setData(QVariant::fromValue<RomBrowserModelData>(data));
}
