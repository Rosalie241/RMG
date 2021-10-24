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
#include "../../Globals.hpp"
#include "Config.hpp"
#include "M64P/Wrapper/Types.hpp"
#include "ColumnID.hpp"

#include <QDir>

using namespace UserInterface::Widget;

static QString columnIdToText(RomInfo_t& romInfo, ColumnID id)
{
    switch (id)
    {
        case ColumnID::GoodName:
        {
            QString name = QString(romInfo.Settings.goodname);
            if (name.contains("(unknown rom)"))
            {
                name = QFileInfo(romInfo.FileName).fileName();
            }
            return name;
        }
        case ColumnID::InternalName:
            return QString((char*)romInfo.Header.Name);
        case ColumnID::MD5:
            return romInfo.Settings.MD5;
        default:
            return "";
    };
}

RomBrowserWidget::RomBrowserWidget(QWidget *parent) : QTableView(parent)
{
    // needed for drag & drop
    this->setDragDropMode(QAbstractItemView::DragDropMode::DropOnly);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(true);

    this->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customContextMenuRequested(QPoint)));

    this->contextMenu_Init();
    this->contextMenu_Setup();

    this->romSearcher_Init();

    this->model_Init();
    this->model_Setup();

    this->widget_Init();

    connect(this->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(on_columnResized(int,int,int)));
}

RomBrowserWidget::~RomBrowserWidget()
{
}

void RomBrowserWidget::RefreshRomList(void)
{
    this->model_Setup();
}

bool RomBrowserWidget::IsRefreshingRomList(void)
{
    return this->romSearcher_Thread->isRunning();
}

void RomBrowserWidget::StopRefreshRomList(void)
{
    if (this->romSearcher_Thread->isRunning())
    {
        this->romSearcher_Thread->terminate();
    }
}

void RomBrowserWidget::SetDirectory(QString directory)
{
    this->directory = directory;
}

void RomBrowserWidget::contextMenu_Init(void)
{
    this->contextMenu_Actions_Init();
    this->contextMenu = new QMenu(this);
}

void RomBrowserWidget::contextMenu_Setup(void)
{
    this->contextMenu_Actions_Setup();
    this->contextMenu_Actions_Connect();

    this->contextMenu->clear();

    this->contextMenu->addAction(this->action_PlayGame);
    this->contextMenu->addAction(this->action_PlayGameWithDisk);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_RefreshRomList);
    this->contextMenu->addAction(this->action_ChooseRomDirectory);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_RomInformation);
    this->contextMenu->addSeparator();
    this->contextMenu->addAction(this->action_EditGameSettings);
    this->contextMenu->addAction(this->action_EditCheats);
}

void RomBrowserWidget::contextMenu_Actions_Init(void)
{
    this->action_PlayGame = new QAction(this);
    this->action_PlayGameWithDisk = new QAction(this);
    this->action_RefreshRomList = new QAction(this);
    this->action_ChooseRomDirectory = new QAction(this);
    this->action_RomInformation = new QAction(this);
    this->action_EditGameSettings = new QAction(this);
    this->action_EditCheats = new QAction(this);
}

void RomBrowserWidget::contextMenu_Actions_Setup(void)
{
    this->action_PlayGame->setText("Play Game");
    this->action_PlayGameWithDisk->setText("Play Game with Disk");
    this->action_RefreshRomList->setText("Refresh ROM List");
    this->action_ChooseRomDirectory->setText("Choose ROM Directory...");
    this->action_RomInformation->setText("ROM Information");
    this->action_EditGameSettings->setText("Edit Game Settings");
    this->action_EditCheats->setText("Edit Cheats");
}

void RomBrowserWidget::contextMenu_Actions_Connect(void)
{
    connect(this->action_PlayGame, &QAction::triggered, this, &RomBrowserWidget::on_Action_PlayGame);
    connect(this->action_PlayGameWithDisk, &QAction::triggered, this, &RomBrowserWidget::on_Action_PlayGameWithDisk);
    connect(this->action_RefreshRomList, &QAction::triggered, this, &RomBrowserWidget::on_Action_RefreshRomList);
    connect(this->action_ChooseRomDirectory, &QAction::triggered, this,
            &RomBrowserWidget::on_Action_ChooseRomDirectory);
    connect(this->action_RomInformation, &QAction::triggered, this, &RomBrowserWidget::on_Action_RomInformation);
    connect(this->action_EditGameSettings, &QAction::triggered, this, &RomBrowserWidget::on_Action_EditGameSettings);
    connect(this->action_EditCheats, &QAction::triggered, this, &RomBrowserWidget::on_Action_EditCheats);
}

void RomBrowserWidget::model_Init(void)
{
    this->model_Model = new QStandardItemModel(this);
    this->model_Model->installEventFilter(this);

    connect(this, &QTableView::doubleClicked, this, &RomBrowserWidget::on_Row_DoubleClicked);
}

void RomBrowserWidget::model_Setup(void)
{
    if (this->romSearcher_Thread->isRunning())
        return;

    this->model_Model->clear();

    this->romSearcher_Launch(this->directory);

    this->model_Columns = CoreSettingsGetIntListValue(SettingsID::RomBrowser_Columns);
    // sanitize list
    for (int column : this->model_Columns)
    {
        if (column >= (int)ColumnID::Invalid || column < 0)
        {
            this->model_Columns.erase(this->model_Columns.begin() + column);
        }
    }

    this->model_Setup_Labels();

    this->column_SetSize();
}

void RomBrowserWidget::model_Setup_Labels(void)
{
    QStringList labels;

    for (int id : this->model_Columns)
    {
        labels.append(g_ColumnTitles[id].Text);
    }

    this->model_Model->setColumnCount(labels.size());
    this->model_Model->setHorizontalHeaderLabels(labels);
}

void RomBrowserWidget::widget_Init(void)
{
    this->widget_Delegate = new NoFocusDelegate(this);

    this->setModel(this->model_Model);
    this->setItemDelegate(this->widget_Delegate);
    this->setWordWrap(false);
    this->setShowGrid(false);
    this->setSortingEnabled(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QTableView::SelectRows);
    this->setSelectionMode(QAbstractItemView::SingleSelection);

    this->verticalHeader()->hide();

    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    this->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    this->horizontalHeader()->setSortIndicatorShown(false);
    this->horizontalHeader()->setHighlightSections(false);

    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

}

void RomBrowserWidget::romSearcher_Init(void)
{
    this->romSearcher_Thread = new Thread::RomSearcherThread(this);

    connect(romSearcher_Thread, &Thread::RomSearcherThread::on_Rom_Found, this,
            &RomBrowserWidget::on_RomBrowserThread_Received);
}

void RomBrowserWidget::romSearcher_Launch(QString directory)
{
    if (directory.isEmpty())
    {
        return;
    }

    this->romSearcher_Thread->SetMaximumFiles(CoreSettingsGetIntValue(SettingsID::RomBrowser_MaxItems));
    this->romSearcher_Thread->SetRecursive(CoreSettingsGetBoolValue(SettingsID::RomBrowser_Recursive));
    this->romSearcher_Thread->SetDirectory(directory);
    this->romSearcher_Thread->start();
}

void RomBrowserWidget::column_SetSize(void)
{
    int index = 0;
    int columnCount = (this->model_Model->columnCount() - 1);

    std::vector<int> sizes = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnSizes);

    for (int id : this->model_Columns)
    {
        // don't set size for the last column
        if (columnCount == index)
        {
            break;
        }

        for (int i = 0; i < sizes.size(); i += 2)
        {
            int columnId = sizes[i];
            if (columnId == id)
            {
                this->setColumnWidth(index++, sizes[i + 1]);
            }
        }
    }
}

int RomBrowserWidget::column_GetSizeSettingIndex(int column)
{
    std::vector<int> sizes = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnSizes);

    for (int id : this->model_Columns)
    {
        // skip irrelevant columns
        if (id != column)
        {
            continue;
        }

        for (int i = 0; i < sizes.size(); i += 2)
        {
            int columnId = sizes[i];
            if (columnId == id)
            {
                return (i + 1);
            }
        }
    }

    return -1;
}

void RomBrowserWidget::launchSelectedRom(void)
{
    QModelIndex index = this->selectedIndexes().first();
    QString rom = this->model()->itemData(index).last().toString();

    emit this->on_RomBrowser_Select(rom);
}

void RomBrowserWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void RomBrowserWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void RomBrowserWidget::dropEvent(QDropEvent *event)
{
    // sadly installing an EventFilter doesn't work
    // on a QTableView, so just override the event,
    // emit the signal and pass it through to QTableView
    emit this->on_RomBrowser_FileDropped(event);
    QTableView::dropEvent(event);
}

void RomBrowserWidget::customContextMenuRequested(QPoint position)
{
    this->contextMenu->popup(this->viewport()->mapToGlobal(position));
}

void RomBrowserWidget::on_columnResized(int column, int oldWidth, int newWidth)
{
    std::vector<int> sizes = CoreSettingsGetIntListValue(SettingsID::RomBrowser_ColumnSizes);

    int sizeIndex = this->column_GetSizeSettingIndex(column);

    // when we've failed to find the setting index,
    // return because that shouldn't happen
    if (sizeIndex == -1)
    {
        return;
    }

    sizes[sizeIndex] = newWidth;

    CoreSettingsSetValue(SettingsID::RomBrowser_ColumnSizes, sizes);
}

void RomBrowserWidget::on_Action_PlayGame(void)
{
    this->launchSelectedRom();
}

void RomBrowserWidget::on_Action_PlayGameWithDisk(void)
{
}

void RomBrowserWidget::on_Action_RefreshRomList(void)
{
    this->RefreshRomList();
}

void RomBrowserWidget::on_Action_ChooseRomDirectory(void)
{
}

void RomBrowserWidget::on_Action_RomInformation(void)
{
}

void RomBrowserWidget::on_Action_EditGameSettings(void)
{
}

void RomBrowserWidget::on_Action_EditCheats(void)
{
}

void RomBrowserWidget::on_Row_DoubleClicked(const QModelIndex &index)
{
    this->launchSelectedRom();
}

void RomBrowserWidget::on_RomBrowserThread_Received(M64P::Wrapper::RomInfo_t romInfo)
{
    QList<QStandardItem *> rowList;

    for (int id : this->model_Columns)
    {
        QStandardItem *item = new QStandardItem();

        item->setText(columnIdToText(romInfo, (ColumnID)id));
        item->setData(romInfo.FileName);
        rowList.append(item);
    }

    this->model_Model->appendRow(rowList);

    this->horizontalHeader()->setStretchLastSection(true);
}
