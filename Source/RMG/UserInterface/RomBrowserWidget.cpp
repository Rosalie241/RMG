#include <UserInterface/RomBrowserWidget.hpp>
#include <Util/Logger.hpp>
#include <Config.hpp>

#include <QDir>

RomBrowserWidget::RomBrowserWidget(void) : QTableView(nullptr)
{
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

void RomBrowserWidget::model_Init(void)
{
    this->model_Model = new QStandardItemModel(this);
}

void RomBrowserWidget::model_Setup(void)
{
    if (this->rom_List_Fill_Thread_Running)
        return;

    this->rom_List_Index = 0;
    this->rom_List_Recursive = false;
    this->model_Model->clear();

    this->rom_List_Fill("/home/rosalie/Downloads/n64_roms/");

    QStringList list;
    list.append("Name");
    list.append("Internal Name");
    list.append("MD5");

  //  this->model_Model->setRowCount(this->rom_List_Index);
    this->model_Model->setColumnCount(list.size());
    this->model_Model->setHorizontalHeaderLabels(list);
}

#include <iostream>
void RomBrowserWidget::widget_Init(void)
{

    this->setModel(this->model_Model);
    this->setShowGrid(false);
    this->setSelectionBehavior(QTableView::SelectRows);
    this->setSortingEnabled(true);

    this->verticalHeader()->hide();

    this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    this->horizontalHeader()->setSortIndicatorShown(false);
    this->horizontalHeader()->setHighlightSections(false);
    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->verticalHeader()->setMaximumSectionSize(20);

    QFont f = this->font();
    f.setPixelSize(11);
    this->setFont(f);
}

void RomBrowserWidget::rom_List_Init()
{
    this->rom_Searcher_Thread = new RomSearcherThread();
    this->rom_Searcher_Thread->SetMaximumFiles(APP_ROMSEARCHER_MAX);

    // TODO
    this->rom_Searcher_Thread->SetRecursive(true);

    connect(rom_Searcher_Thread, &RomSearcherThread::on_Rom_Found, this, &RomBrowserWidget::on_RomBrowserThread_Received);
    connect(rom_Searcher_Thread, &RomSearcherThread::finished, this, &RomBrowserWidget::on_RomBrowserThread_Finished);
}

void RomBrowserWidget::rom_List_Fill(QString directory)
{
    if (!this->rom_List_Fill_Thread_Init)
        this->rom_List_Init();

    if (this->rom_List_Fill_Thread_Running)
        return;

    this->rom_List_Fill_Thread_Running = true;
    this->rom_Searcher_Thread->SetDirectory(directory);
    this->rom_Searcher_Thread->start();
}

void RomBrowserWidget::on_RomBrowserThread_Received(m64p_rom_info romInfo)
{
    QList<QStandardItem*> list;

    list.append(new QStandardItem(romInfo.m64p_settings.goodname));
    list.append(new QStandardItem(QString((char*)romInfo.m64p_header.Name)));
    list.append(new QStandardItem(romInfo.m64p_settings.MD5));

    this->model_Model->appendRow(list);

    this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void RomBrowserWidget::on_RomBrowserThread_Finished(void)
{
    this->rom_List_Fill_Thread_Running = false;
}
