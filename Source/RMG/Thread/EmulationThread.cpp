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

#include <RMG-Core/Emulation.hpp>
#include <RMG-Core/Error.hpp>

#ifdef _WIN32
#include <Windows.h>
#else // Linux
#include <QDBusConnection>
#include <QDBusReply>
#endif

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

void EmulationThread::SetNetplay(QString address, int port, int player)
{
    this->address = address;
    this->port    = port;
    this->player  = player;
}

void EmulationThread::run(void)
{
    this->inhibitScreensaver();

    emit this->on_Emulation_Started();

    bool ret = CoreStartEmulation(this->rom.toStdU32String(), this->disk.toStdU32String(), 
                                  this->address.toStdString(), this->port, this->player);

    emit this->on_Emulation_Finished(ret, QString::fromStdString(CoreGetError()));

    this->resetState();
    this->uninhibitScreensaver();
}

void EmulationThread::resetState(void)
{
    this->rom.clear();
    this->disk.clear();
    this->address.clear();
    this->port   = -1;
    this->player = -1;
}

void EmulationThread::inhibitScreensaver(void)
{
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
#else
    this->dbusCookieId = 0;
    this->dbusInterface = new QDBusInterface("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver", QDBusConnection::sessionBus());
    if (!this->dbusInterface->isValid())
    {
        return;
    }

    QDBusReply<uint32_t> dbusReply = this->dbusInterface->call("Inhibit", "RMG", "game");
    if (dbusReply.isValid())
    {
        this->dbusCookieId = dbusReply.value();
    }
#endif
}

void EmulationThread::uninhibitScreensaver(void)
{
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#else
    if (this->dbusInterface == nullptr)
    {
        return;
    }

    if (this->dbusCookieId != 0)
    {
        this->dbusInterface->call("UnInhibit", this->dbusCookieId);
    }

    this->dbusInterface->deleteLater();
    this->dbusInterface = nullptr;
#endif
}
