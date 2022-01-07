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
#include "RMG-Core/Rom.hpp"

#include <QDir>
#include <QDirIterator>

using namespace Thread;

RomSearcherThread::RomSearcherThread(QObject *parent) : QThread(parent)
{
    qRegisterMetaType<CoreRomHeader>("CoreRomHeader");
    qRegisterMetaType<CoreRomSettings>("CoreRomSettings");
}

RomSearcherThread::~RomSearcherThread(void)
{
    this->Stop();
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

void RomSearcherThread::Stop(void)
{
    this->rom_Search_Stop = true;
    while (this->isRunning())
    {
        this->wait();
    }
}

void RomSearcherThread::run(void)
{
    this->rom_Search_Stop = false;
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

    QDirIterator::IteratorFlag flag = this->rom_Search_Recursive ? 
        QDirIterator::Subdirectories : 
        QDirIterator::NoIteratorFlags;
    QDirIterator romDirIt(directory, filter, QDir::Files, flag);

    CoreRomHeader   header;
    CoreRomSettings settings;
    bool            ret;
    int             count = 0;

    while (romDirIt.hasNext())
    {
        QString file = romDirIt.next();

        // open rom, retrieve rom settings & header 
        ret = CoreOpenRom(file.toStdString()) && 
            CoreGetCurrentRomSettings(settings) && 
            CoreGetCurrentRomHeader(header);
        // always close the ROM,
        // even when retrieving rom info failed
        ret = CoreCloseRom() && ret;
        
        if (ret)
        {
            if (count++ >= this->rom_Search_MaxItems)
            {
                return;
            }

            emit this->on_Rom_Found(file, header, settings);
        }

        if (this->rom_Search_Stop)
        {
            break;
        }
    }
}

