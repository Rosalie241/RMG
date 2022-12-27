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
#include <QElapsedTimer>

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
    this->directory = directory;
}

void RomSearcherThread::SetRecursive(bool value)
{
    this->recursive = value;
}

void RomSearcherThread::SetMaximumFiles(int value)
{
    this->maxItems = value;
}

void RomSearcherThread::Stop(void)
{
    this->stop = true;
    while (this->isRunning())
    {
        this->wait();
    }
}

void RomSearcherThread::run(void)
{
    this->stop = false;
    this->searchDirectory(this->directory);
    return;
}

void RomSearcherThread::searchDirectory(QString directory)
{
    QDir dir(directory);

    QStringList filter;
    filter << "*.N64";
    filter << "*.Z64";
    filter << "*.V64";
    filter << "*.NDD";
    filter << "*.D64";
    filter << "*.ZIP";

    QDirIterator::IteratorFlag flag = this->recursive ? 
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

        QElapsedTimer timer;
        timer.start();

        if (CoreHasRomHeaderAndSettingsCached(file.toStdU32String()))
        { // found cache entry
            ret = CoreGetCachedRomHeaderAndSettings(file.toStdU32String(), header, settings);
        }
        else
        { // no cache entry
            // open rom, retrieve rom settings & header 
            ret = CoreOpenRom(file.toStdU32String()) && 
                CoreGetCurrentRomSettings(settings) && 
                CoreGetCurrentRomHeader(header);
            // always close the ROM,
            // even when retrieving rom info failed
            ret = CoreCloseRom() && ret;
            if (ret)
            { // add to cache when everything succeeded
                CoreAddCachedRomHeaderAndSettings(file.toStdU32String(), header, settings);
            }
        }
        
        if (ret)
        {
            if (count++ >= this->maxItems)
            {
                return;
            }

            emit this->RomFound(file, header, settings);
        }

        if (this->stop)
        {
            break;
        }

        // ensure the UI thread has some breathing room
        // instead of being locked up and unable to render anything
        if (timer.elapsed() < 10)
        {
            QThread::msleep(10 - timer.elapsed());
        }
    }

    emit this->Finished(this->stop);
}

