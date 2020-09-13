#include <Thread/RomSearcherThread.hpp>
#include <Mupen/Mupen.hpp>
#include <Util/Logger.hpp>

#include <QDir>

RomSearcherThread::RomSearcherThread(void)
{
    qRegisterMetaType<m64p_rom_info>("m64p_rom_info");
}

void RomSearcherThread::SetDirectory(QString directory)
{
    this->rom_Directory = directory;
}

void RomSearcherThread::SetRecursive(bool value)
{
    this->rom_Search_Recursive = value;
}

void RomSearcherThread::SetMaximumFiles(int value)
{
    this->rom_Search_MaxItems = value;
}

void RomSearcherThread::run()
{
    this->rom_Search_Count = 0;
    this->rom_Search(this->rom_Directory);
    return;
}

void RomSearcherThread::rom_Search(QString directory)
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
            if (this->rom_Search_Count >= this->rom_Search_MaxItems)
                return;

            this->rom_Search_Count++;

            emit this->on_Rom_Found(romInfo);
        }
    }

    if (this->rom_Search_Recursive)
    {
        fileList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (int i = 0; i < fileList.size(); i++)
        {
            this->rom_Search(fileList.at(i).absoluteFilePath());
        }
    }
}

bool RomSearcherThread::rom_Get_Info(QString file, m64p_rom_info *info)
{
    m64p_rom_header header;
    m64p_rom_settings settings;

    if (!g_MupenApi.GetRomInfo(file, &header, &settings))
    {
        g_Logger.AddText("RomSearcherThread::rom_Get_Info: " + g_MupenApi.GetLastError());
        return false;
    }

    info->file = file;
    info->m64p_header = header;
    info->m64p_settings = settings;

    return true;
}
