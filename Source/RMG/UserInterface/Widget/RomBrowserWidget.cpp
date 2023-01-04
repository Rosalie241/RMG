/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomBrowserWidget.hpp"

#include <RMG-Core/Core.hpp>

#include <QDir>
#include <QBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QLabel>
#include <QPixmap>
#include <vector>
#include <QList>
#include <QScrollBar>
#include <QDesktopServices>

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

RomBrowserWidget::RomBrowserWidget(QWidget *parent) : QStackedWidget(parent)
{
    // configure signal types
    qRegisterMetaType<CoreRomType>("CoreRomType");

    // configure rom searcher thread
    this->romSearcherThread = new Thread::RomSearcherThread(this);
    connect(this->romSearcherThread, &Thread::RomSearcherThread::RomFound, this, &RomBrowserWidget::on_RomBrowserThread_RomFound);
    connect(this->romSearcherThread, &Thread::RomSearcherThread::Finished, this, &RomBrowserWidget::on_RomBrowserThread_Finished);

    // configure empty widget
    this->emptyWidget = new Widget::RomBrowserEmptyWidget(this);
    this->addWidget(this->emptyWidget);
    connect(this->emptyWidget, &RomBrowserEmptyWidget::SelectRomDirectory, this, &RomBrowserWidget::on_Action_ChangeRomDirectory);
    connect(this->emptyWidget, &RomBrowserEmptyWidget::Refresh, this, &RomBrowserWidget::on_Action_RefreshRomList);

    // configure loading widget
    this->loadingWidget = new Widget::RomBrowserLoadingWidget(this);
    connect(this, &QStackedWidget::currentChanged, this->loadingWidget, &RomBrowserLoadingWidget::on_RomBrowserWidget_currentChanged);
    this->loadingWidget->SetWidgetIndex(this->addWidget(this->loadingWidget));

    // configure list view widget
    this->listViewWidget = new Widget::RomBrowserListViewWidget(this);
    this->listViewModel  = new QStandardItemModel(this);
    this->listViewWidget->setModel(this->listViewModel);
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
    this->listViewWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    this->listViewWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    this->listViewWidget->horizontalHeader()->setStretchLastSection(true);
    this->listViewWidget->horizontalHeader()->setSortIndicatorShown(false);
    this->listViewWidget->horizontalHeader()->setHighlightSections(false);
    this->listViewWidget->horizontalHeader()->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    this->addWidget(this->listViewWidget);
    connect(this->listViewWidget, &QTableView::doubleClicked, this, &RomBrowserWidget::on_DoubleClicked);
    connect(this->listViewWidget, &QTableView::customContextMenuRequested, this, &RomBrowserListViewWidget::customContextMenuRequested);
    connect(this->listViewWidget->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &RomBrowserWidget::on_listViewWidget_sortIndicatorChanged);
    connect(this->listViewWidget->horizontalHeader(), &QHeaderView::sectionResized, this, &RomBrowserWidget::on_listViewWidget_sectionResized);
    connect(this->listViewWidget, &Widget::RomBrowserListViewWidget::ZoomIn, this, &RomBrowserWidget::on_ZoomIn);
    connect(this->listViewWidget, &Widget::RomBrowserListViewWidget::ZoomOut, this, &RomBrowserWidget::on_ZoomOut);

    // TODO
    QStringList labels;
    labels << "Name";
    labels << "Internal Name";
    labels << "MD5";
    this->listViewModel->setColumnCount(labels.size());
    this->listViewModel->setHorizontalHeaderLabels(labels);

    // configure grid view widget
    this->gridViewWidget = new Widget::RomBrowserGridViewWidget(this);
    this->gridViewModel  = new QStandardItemModel(this);
    this->gridViewWidget->setModel(this->gridViewModel);
    this->gridViewWidget->setFlow(QListView::Flow::LeftToRight);
    this->gridViewWidget->setResizeMode(QListView::Adjust);
    this->gridViewWidget->setMovement(QListView::Static);
    this->gridViewWidget->setUniformItemSizes(CoreSettingsGetBoolValue(SettingsID::RomBrowser_GridViewUniformItemSizes));
    this->gridViewWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->gridViewWidget->setViewMode(QListView::ViewMode::IconMode);
    this->gridViewWidget->setTextElideMode(Qt::TextElideMode::ElideNone);    
    this->gridViewWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->gridViewWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->gridViewWidget->verticalScrollBar()->setSingleStep(15);
    this->gridViewWidget->setWordWrap(true);
    this->gridViewWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    this->gridViewWidget->setFrameStyle(QFrame::NoFrame);
    int iconWidth = CoreSettingsGetIntValue(SettingsID::RomBrowser_GridViewIconWidth);
    int iconHeight = CoreSettingsGetIntValue(SettingsID::RomBrowser_GridViewIconHeight);
    this->gridViewWidget->setIconSize(QSize(iconWidth, iconHeight));
    this->addWidget(this->gridViewWidget);
    connect(this->gridViewWidget, &QListView::doubleClicked, this, &RomBrowserWidget::on_DoubleClicked);
    connect(this->gridViewWidget, &QListView::customContextMenuRequested, this, &RomBrowserListViewWidget::customContextMenuRequested);
    connect(this->gridViewWidget, &QListView::iconSizeChanged, this, &RomBrowserWidget::on_gridViewWidget_iconSizeChanged);
    connect(this->gridViewWidget, &Widget::RomBrowserGridViewWidget::ZoomIn, this, &RomBrowserWidget::on_ZoomIn);
    connect(this->gridViewWidget, &Widget::RomBrowserGridViewWidget::ZoomOut, this, &RomBrowserWidget::on_ZoomOut);

    // configure context menu policy
    this->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(this, &QStackedWidget::customContextMenuRequested, this, &RomBrowserWidget::customContextMenuRequested);

    // configure context menu actions
    this->action_PlayGame = new QAction(this);
    this->action_PlayGameWith = new QAction(this);
    this->action_RefreshRomList = new QAction(this);
    this->action_OpenRomDirectory = new QAction(this);
    this->action_ChangeRomDirectory = new QAction(this);
    this->action_RomInformation = new QAction(this);
    this->action_EditGameSettings = new QAction(this);
    this->action_EditCheats = new QAction(this);
    this->action_SetCoverImage = new QAction(this);
    this->action_RemoveCoverImage = new QAction(this);
    this->action_PlayGame->setText("Play Game");
    this->action_PlayGameWith->setText("Play Game with Disk");
    this->action_RefreshRomList->setText("Refresh ROM List");
    this->action_OpenRomDirectory->setText("Open ROM Directory");
    this->action_ChangeRomDirectory->setText("Change ROM Directory...");
    this->action_RomInformation->setText("ROM Information");
    this->action_EditGameSettings->setText("Edit Game Settings");
    this->action_EditCheats->setText("Edit Cheats");
    this->action_SetCoverImage->setText("Set Cover Image...");
    this->action_RemoveCoverImage->setText("Remove Cover Image");
    connect(this->action_PlayGame, &QAction::triggered, this, &RomBrowserWidget::on_Action_PlayGame);
    connect(this->action_PlayGameWith, &QAction::triggered, this, &RomBrowserWidget::on_Action_PlayGameWith);
    connect(this->action_RefreshRomList, &QAction::triggered, this, &RomBrowserWidget::on_Action_RefreshRomList);
    connect(this->action_OpenRomDirectory, &QAction::triggered, this, &RomBrowserWidget::on_Action_OpenRomDirectory);
    connect(this->action_ChangeRomDirectory, &QAction::triggered, this, &RomBrowserWidget::on_Action_ChangeRomDirectory);
    connect(this->action_RomInformation, &QAction::triggered, this, &RomBrowserWidget::on_Action_RomInformation);
    connect(this->action_EditGameSettings, &QAction::triggered, this, &RomBrowserWidget::on_Action_EditGameSettings);
    connect(this->action_EditCheats, &QAction::triggered, this, &RomBrowserWidget::on_Action_EditCheats);
    connect(this->action_SetCoverImage, &QAction::triggered, this, &RomBrowserWidget::on_Action_SetCoverImage);
    connect(this->action_RemoveCoverImage, &QAction::triggered, this, &RomBrowserWidget::on_Action_RemoveCoverImage);

    // configure context menu
    this->contextMenu = new QMenu(this);
    this->contextMenu->addAction(this->action_PlayGame);
    this->contextMenu->addAction(this->action_PlayGameWith);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_RefreshRomList);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_OpenRomDirectory);
    this->contextMenu->addAction(this->action_ChangeRomDirectory);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_RomInformation);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_EditGameSettings);
    this->contextMenu->addAction(this->action_EditCheats);
    this->contextMenu->addSeparator();
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

    this->coversDirectory = QString::fromStdString(CoreGetUserDataDirectory().string());
    this->coversDirectory += "/Covers";

    this->listViewSortSection = CoreSettingsGetIntValue(SettingsID::RomBrowser_ListViewSortSection);
    this->listViewSortOrder   = CoreSettingsGetIntValue(SettingsID::RomBrowser_ListViewSortOrder);

    QString directory = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::RomBrowser_Directory));
    if (directory.isEmpty())
    {
        this->setCurrentWidget(this->emptyWidget);
        return;
    }

    this->setCurrentWidget(this->loadingWidget);
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
        this->currentWidget() != this->emptyWidget)
    {
        this->setCurrentWidget(this->listViewWidget);        
    }
}

void RomBrowserWidget::ShowGrid(void)
{
    this->currentViewWidget = this->gridViewWidget;

    // only change widget now when we're not refreshing
    if (!this->IsRefreshingRomList() &&
        this->currentWidget() != this->emptyWidget)
    {
        this->setCurrentWidget(this->gridViewWidget);        
    }
}

void RomBrowserWidget::SetGridViewUniformSizes(bool value)
{
    // refresh ROM list when we're currently in the grid view
    // and we're not refreshing already
    if (this->currentWidget() == this->gridViewWidget &&
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

QStandardItemModel* RomBrowserWidget::getCurrentModel(void)
{
    QWidget* currentWidget = this->currentWidget();
    if (currentWidget == this->listViewWidget)
    {
        return this->listViewModel;
    }
    else if (currentWidget == this->gridViewWidget)
    {
        return this->gridViewModel;
    }

    return nullptr;
}

QAbstractItemView* RomBrowserWidget::getCurrentModelView(void)
{
    QWidget* currentWidget = this->currentWidget();
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
    QStandardItemModel* model = this->getCurrentModel();
    QAbstractItemView*  view  = this->getCurrentModelView();

    if (model == nullptr || view == nullptr)
    {
        return false;
    }

    QModelIndex         index = view->currentIndex();
    QStandardItem*      item  = model->item(index.row(), index.column());

    if (item == nullptr)
    {
        return false;
    }

    data  = model->itemData(index).last().value<RomBrowserModelData>();
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

QIcon RomBrowserWidget::getCurrentCover(CoreRomHeader header, CoreRomSettings settings, QString& coverFileName)
{
    QPixmap pixmap;

    // try to load cover using
    // 1) MD5
    // 2) good name
    // 3) internal name
    bool foundCover = false;
    for (QString name : { 
        QString::fromStdString(settings.MD5), 
        QString::fromStdString(settings.GoodName), 
        QString::fromStdString(header.Name) })
    {
        // fixup file name
        QString fixedName = name;
        for (const char c : ":<>\"/\\|?*")
        {
            // skip empty characters
            if (c == '\0')
            {
                continue;
            }

            fixedName.replace(c, "_");
        }

        // we support jpg & png as file extensions
        for (QString ext : { ".png", ".jpg", ".jpeg" })
        {
            QString coverPath = this->coversDirectory;
            coverPath += "/";
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
    this->setCurrentWidget(this->currentViewWidget);
}

void RomBrowserWidget::on_DoubleClicked(const QModelIndex& index)
{
    emit this->PlayGame(this->getCurrentRom());
}

void RomBrowserWidget::customContextMenuRequested(QPoint position)
{
    QStandardItemModel* model = this->getCurrentModel();
    QAbstractItemView*  view = this->getCurrentModelView();
    if (view == nullptr || model == nullptr)
    {
        return;
    }

    RomBrowserModelData data;
    bool hasSelection = view->selectionModel()->hasSelection();

    this->getCurrentData(data);

    this->action_PlayGame->setEnabled(hasSelection);
    this->action_PlayGameWith->setEnabled(hasSelection);
    this->action_RomInformation->setEnabled(hasSelection);
    this->action_EditGameSettings->setEnabled(hasSelection);
    this->action_EditCheats->setEnabled(hasSelection);
    this->action_SetCoverImage->setEnabled(hasSelection);
    this->action_SetCoverImage->setVisible(view == this->gridViewWidget);
    this->action_RemoveCoverImage->setEnabled(hasSelection && !data.coverFile.isEmpty());
    this->action_RemoveCoverImage->setVisible(view == this->gridViewWidget);

    if (hasSelection && data.type == CoreRomType::Disk)
    { // disk selected
        this->action_PlayGameWith->setText("Play Game with Cartridge");
    }
    else
    { // cartridge selected
        this->action_PlayGameWith->setText("Play Game with Disk");
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

void RomBrowserWidget::on_listViewWidget_sortIndicatorChanged(int logicalIndex, Qt::SortOrder sortOrder)
{
    CoreSettingsSetValue(SettingsID::RomBrowser_ListViewSortSection, logicalIndex);
    CoreSettingsSetValue(SettingsID::RomBrowser_ListViewSortOrder, (int)sortOrder);
}

void RomBrowserWidget::on_listViewWidget_sectionResized(int logicalIndex, int oldWidth, int newWidth)
{
    std::vector<int> columnSizes = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnSizes);

    if (columnSizes.size() <= logicalIndex)
    {
        columnSizes.push_back(newWidth);
    }
    else
    {
        columnSizes.at(logicalIndex) = newWidth;
    }

    CoreSettingsSetValue(SettingsID::RomBrowser_ColumnSizes, columnSizes);
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

void RomBrowserWidget::on_RomBrowserThread_RomFound(QString file, CoreRomType type, CoreRomHeader header, CoreRomSettings settings)
{
    QString name;
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

    // retrieve cover image
    coverIcon = this->getCurrentCover(header, settings, coverFile);
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
    listViewItem2->setData(itemData);
    listViewRow.append(listViewItem2);
    // MD5
    QStandardItem* listViewItem3 = new QStandardItem();
    listViewItem3->setText(QString::fromStdString(settings.MD5));
    listViewItem3->setData(itemData);
    listViewRow.append(listViewItem3);
    this->listViewModel->appendRow(listViewRow);

    QStandardItem* gridViewItem = new QStandardItem();
    gridViewItem->setIcon(coverIcon);
    gridViewItem->setText(name);
    gridViewItem->setData(itemData);
    this->gridViewModel->appendRow(gridViewItem);
}

void RomBrowserWidget::on_RomBrowserThread_Finished(bool canceled)
{
    // sort data
    this->listViewModel->sort(this->listViewSortSection, (Qt::SortOrder)this->listViewSortOrder);
    this->gridViewModel->sort(0, Qt::SortOrder::AscendingOrder);

    // resize list view's columns
    std::vector<int> columnSizes = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnSizes);
    if (columnSizes.size() != this->listViewModel->columnCount() ||
        columnSizes.at(0) == -1)
    {
        // reset column sizes setting
        columnSizes.clear();
        CoreSettingsSetValue(SettingsID::RomBrowser_ColumnSizes, columnSizes);

        this->listViewWidget->resizeColumnsToContents();
    }
    else
    {
        // use settings' width
        for (int i = 0; i < (columnSizes.size() - 1); i++)
        {
            this->listViewWidget->setColumnWidth(i, columnSizes.at(i));
        }
    }

    if (!canceled)
    {
        if (this->listViewModel->rowCount() == 0)
        {
            this->setCurrentWidget(this->emptyWidget);
            return;
        }
    }

    // prevent flicker by forcing the loading screen
    // to be shown at least 300ms
    qint64 elapsedTime = this->romSearcherTimer.elapsed();
    if (elapsedTime < 300)
    {
        this->startTimer(300 - elapsedTime);
        return;
    }

    this->setCurrentWidget(this->currentViewWidget);
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

    QDesktopServices::openUrl(QUrl(directory));
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

void RomBrowserWidget::on_Action_EditCheats(void)
{
    emit this->Cheats(this->getCurrentRom());
}

void RomBrowserWidget::on_Action_SetCoverImage(void)
{
    QString sourceFile;
    QFileInfo sourceFileInfo;
    QString coverFile;

    QStandardItemModel* model = this->getCurrentModel();
    QAbstractItemView*  view  = this->getCurrentModelView();
    if (model == nullptr || view == nullptr)
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

    QModelIndex         index = view->currentIndex();
    QStandardItem*      item  = model->item(index.row(), index.column());
    RomBrowserModelData data  = model->itemData(index).last().value<RomBrowserModelData>();

    // construct new file name (for the cover)
    QString newFileName = this->coversDirectory;
    newFileName += "/";
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
    item->setIcon(this->getCurrentCover(data.header, data.settings, coverFile));
    data.coverFile = coverFile;
    item->setData(QVariant::fromValue<RomBrowserModelData>(data));
}

void RomBrowserWidget::on_Action_RemoveCoverImage(void)
{
    QStandardItemModel* model = this->getCurrentModel();
    QAbstractItemView*  view  = this->getCurrentModelView();
    if (model == nullptr || view == nullptr)
    {
        return;
    }

    QModelIndex         index = view->currentIndex();
    QStandardItem*      item  = model->item(index.row(), index.column());
    RomBrowserModelData data  = model->itemData(index).last().value<RomBrowserModelData>();
    QString coverFile;

    if (!data.coverFile.isEmpty() && QFile::exists(data.coverFile))
    {
        QFile::remove(data.coverFile);
    }

    // update item
    item->setIcon(this->getCurrentCover(data.header, data.settings, coverFile));
    data.coverFile = coverFile;
    item->setData(QVariant::fromValue<RomBrowserModelData>(data));
}
