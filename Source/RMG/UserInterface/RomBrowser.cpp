#include "RomBrowser.hpp"

#include <QDir>

RomBrowser::RomBrowser(Mupen* m64p) : QTableView(nullptr)
{
    this->m64p = m64p;
    this->model_Init();
    this->model_Setup();
    this->widget_Init();

}

RomBrowser::~RomBrowser()
{
}

void RomBrowser::RefreshRomList(void)
{
    this->model_Setup();
}

void RomBrowser::model_Init(void)
{
    this->model_Model = new QStandardItemModel(this);
}

void RomBrowser::model_Setup(void)
{
    this->rom_List_Index = 0;
    this->model_Model->clear();

    this->rom_List_Fill("/home/rosalie/Downloads/n64_roms/");

    QStringList list;
    list.append("Name");
    list.append("Internal Name");
    list.append("MD5");

    this->model_Model->setRowCount(this->rom_List_Index);
    this->model_Model->setColumnCount(list.size());
    this->model_Model->setHorizontalHeaderLabels(list);

    m64p_rom_info romInfo;

    for (int row = 0; row < model_Model->rowCount(); row++)
    {
        romInfo = this->rom_List[row];

        for (int column = 0; column < model_Model->columnCount(); column++)
        {
            QStandardItem *item;
            if (column == 0)
            {
                item = new QStandardItem(romInfo.m64p_settings.goodname);
            }
            if (column == 1)
            {
                item = new QStandardItem(QString((char*)romInfo.m64p_header.Name));
            }
            if (column == 2)
            {
                item = new QStandardItem(romInfo.m64p_settings.MD5);
            }

            this->model_Model->setItem(row, column, item);
        }
    }
}

void RomBrowser::widget_Init(void)
{
    this->setModel(this->model_Model);
    this->setShowGrid(false);
    this->setSelectionBehavior(QTableView::SelectRows);
    this->setSortingEnabled(true);

    this->verticalHeader()->hide();

    this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->horizontalHeader()->setSectionResizeMode(this->model_Model->columnCount() - 1, QHeaderView::Stretch);
    this->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
}

void RomBrowser::rom_List_Fill(QString directory)
{
    QDir dir(directory);

    QStringList filter;
    filter << "*.N64";
    filter << "*.Z64";
    filter << "*.V64";

    QFileInfoList fileList = dir.entryInfoList(filter, QDir::Files);
    QFileInfo fileInfo;
    m64p_rom_info romInfo;
    bool ret;

    for (int i = 0; i < fileList.size(); i++)
    {
        fileInfo = fileList.at(i);
        
        ret = this->rom_Get_Info(fileInfo.absoluteFilePath(), &romInfo);
        if (ret)
        {
            if (rom_List_Index >= MAX_ROM_INFO)
                return;

            this->rom_List[rom_List_Index++] = romInfo;
        }
    }

    if (this->rom_List_Recursive)
    {
        fileList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (int i = 0; i < fileList.size(); i++)
        {
            this->rom_List_Fill(fileList.at(i).absoluteFilePath());
        }
    }
}


bool RomBrowser::rom_Get_Info(QString file, m64p_rom_info* info)
{
    m64p_rom_header header;
    m64p_rom_settings settings;

    if (!this->m64p->GetRomInfo(file, &header, &settings))
        return false;

    info->file = file;
    info->m64p_header = header;
    info->m64p_settings = settings;

    return true;
}