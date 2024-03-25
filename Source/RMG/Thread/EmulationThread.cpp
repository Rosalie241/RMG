/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "EmulationThread.hpp"
#include <RMG-Core/Core.hpp>

using namespace Thread;

EmulationThread::EmulationThread(QObject *parent) : QThread(parent)
{
#ifndef _WIN32
    // on Linux, musl has a too small stack size,
    // causing a crash in mesa when using paraLLEl,
    // so to fix it, set a stack size of 2MiB for
    // the emulation thread
    // see https://github.com/Rosalie241/RMG/issues/219
    this->setStackSize(0x200000);
#endif
}

EmulationThread::~EmulationThread(void)
{
}

void EmulationThread::SetRomFile(QString file)
{
    this->rom = file;
}

void EmulationThread::SetDiskFile(QString file)
{
    this->disk = file;
}

void EmulationThread::run(void)
{
    emit this->on_Emulation_Started();

    bool ret = CoreStartEmulation(this->rom.toStdU32String(), this->disk.toStdU32String());
    if (!ret)
    {
        this->errorMessage = QString::fromStdString(CoreGetError());
    }

    emit this->on_Emulation_Finished(ret);
}

QString EmulationThread::GetLastError(void)
{
    return this->errorMessage;
}
