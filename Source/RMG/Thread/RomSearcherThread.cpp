/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomSearcherThread.hpp"
#include "../Globals.hpp"

#include <QDir>

using namespace Thread;

RomSearcherThread::RomSearcherThread(QObject *parent) : QThread(parent)
{
    qRegisterMetaType<M64P::Wrapper::RomInfo_t>("M64P::Wrapper::RomInfo_t");
}

RomSearcherThread::~RomSearcherThread(void)
{
    // kill the thread when we're running
    if (this->isRunning())
    {
        this->terminate();
        this->wait();
    }
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

void RomSearcherThread::run(void)
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
    filter << "*.NDD";
    filter << "*.D64";
    filter << "*.ZIP";

    QFileInfoList fileList = dir.entryInfoList(filter, QDir::Files);
    QFileInfo fileInfo;
    M64P::Wrapper::RomInfo_t romInfo;
    bool ret;

    for (int i = 0; i < fileList.size(); i++)
    {
        fileInfo = fileList.at(i);

        ret = this->rom_Get_Info(fileInfo.absoluteFilePath(), &romInfo);
        if (ret)
        {
            if (this->rom_Search_Count++ >= this->rom_Search_MaxItems)
                return;

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

bool RomSearcherThread::rom_Get_Info(QString file, M64P::Wrapper::RomInfo_t *info)
{
    return g_MupenApi.Core.GetRomInfo(file, info, true);
}
