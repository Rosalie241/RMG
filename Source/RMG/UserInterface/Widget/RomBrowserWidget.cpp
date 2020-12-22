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

#include <QDir>

using namespace UserInterface::Widget;

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

    this->model_Init();
    this->model_Setup();
    this->widget_Init();
}

RomBrowserWidget::~RomBrowserWidget()
{
}

void RomBrowserWidget::RefreshRomList(void)
{
    this->model_Setup();
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
    connect(this->action_ChooseRomDirectory, &QAction::triggered, this, &RomBrowserWidget::on_Action_ChooseRomDirectory);
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
    if (this->rom_List_Fill_Thread_Running)
        return;

    this->rom_List_Index = 0;
    this->rom_List_Recursive = false;
    this->model_Model->clear();

    if (!this->directory.isEmpty())
        this->rom_List_Fill(this->directory);

    this->model_LabelList_Setup();

    this->model_Model->setColumnCount(this->model_LabelList.size());
    this->model_Model->setHorizontalHeaderLabels(this->model_LabelList);
}

void RomBrowserWidget::model_LabelList_Setup(void)
{
    this->model_LabelList.clear();
    this->model_LabelList.append("Name");
    this->model_LabelList.append("Internal Name");
    this->model_LabelList.append("MD5");
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

    this->column_SetSize();
}

void RomBrowserWidget::rom_Searcher_Init(void)
{
    this->rom_Searcher_Thread = new Thread::RomSearcherThread();
    this->rom_Searcher_Thread->SetMaximumFiles(APP_ROMSEARCHER_MAX);

    // TODO
    this->rom_Searcher_Thread->SetRecursive(true);

    connect(rom_Searcher_Thread, &Thread::RomSearcherThread::on_Rom_Found, this,
            &RomBrowserWidget::on_RomBrowserThread_Received);
    connect(rom_Searcher_Thread, &Thread::RomSearcherThread::finished, this,
            &RomBrowserWidget::on_RomBrowserThread_Finished);
}

bool init = false;

void RomBrowserWidget::rom_List_Fill(QString directory)
{
    if (!init)
    {
        this->rom_Searcher_Init();
        init = true;
    }

    if (this->rom_List_Fill_Thread_Running)
        return;

    this->rom_List_Fill_Thread_Running = true;
    this->rom_Searcher_Thread->SetDirectory(directory);
    this->rom_Searcher_Thread->start();
}

void RomBrowserWidget::column_SetSize(void)
{
    for (int i = 0; i < this->model_LabelList.size(); i++)
    {
        QString label = this->model_LabelList.at(i);

        int oldSize = this->columnWidth(i);
        int newSize = 0;
        if (label == "Name")
        {
            newSize = 250;
        }
        else if (label == "Internal Name")
        {
            newSize = 100;
        }
        else if (label == "MD5")
        {
            newSize = 100;
        }

        this->setColumnWidth(i, newSize);
        continue;
        if (oldSize != newSize)
        {
            this->setColumnWidth(i, oldSize);
        }
        else
        {
            this->setColumnWidth(i, newSize);
        }
    }
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

void RomBrowserWidget::on_Action_PlayGame(void)
{

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

#include <iostream>
void RomBrowserWidget::on_Row_DoubleClicked(const QModelIndex &index)
{
    const M64P::Wrapper::RomInfo_t *info = &this->rom_List.at(index.row());
    std::cout << "aaa " << index.row() << std::endl;
    std::cout << info->FileName.toStdString() << std::endl;
    emit this->on_RomBrowser_Select(info->FileName);
}

void RomBrowserWidget::on_RomBrowserThread_Received(M64P::Wrapper::RomInfo_t romInfo)
{
    QList<QStandardItem *> list;

    this->rom_List.append(romInfo);

    list.append(new QStandardItem(romInfo.Settings.goodname));
    list.append(new QStandardItem(QString((char *)romInfo.Header.Name)));
    list.append(new QStandardItem(romInfo.Settings.MD5));

    this->model_Model->appendRow(list);

    this->column_SetSize();

    this->horizontalHeader()->setStretchLastSection(true);
}

void RomBrowserWidget::on_RomBrowserThread_Finished(void)
{
    this->rom_List_Fill_Thread_Running = false;
}
