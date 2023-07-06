/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "HotkeysThread.hpp"

#include <RMG-Core/Core.hpp>

using namespace Thread;

HotkeysThread::HotkeysThread(std::function<void(int)> checkHotkeysFunc, QObject *parent) : QThread(parent)
{
    this->checkHotkeysFunc = checkHotkeysFunc;
}

HotkeysThread::~HotkeysThread()
{
    if (this->isRunning())
    {
        this->StopLoop();
    }
}

void HotkeysThread::SetState(HotkeysThreadState state)
{
    this->state = state;
}

void HotkeysThread::StopLoop(void)
{
    this->keepLoopRunning = false;
    while (this->isRunning())
    {
        // wait until we're not running anymore
    }
}

void HotkeysThread::run(void)
{
    while (this->keepLoopRunning)
    {
        // sleep for 300ms when no ROM is opened
        if (this->state == HotkeysThreadState::RomClosed)
        {
            QThread::msleep(300);
            continue;
        }

        if (CoreIsEmulationPaused())
        {
            for (int i = 0; i < 4; i++)
            {
                this->checkHotkeysFunc(i);
            }
        }

        // sleep for 100ms
        QThread::msleep(100);
    }
}
