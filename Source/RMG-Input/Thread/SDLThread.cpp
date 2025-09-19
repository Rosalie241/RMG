/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "SDLThread.hpp"

#include <SDL3/SDL.h>

using namespace Thread;

SDLThread::SDLThread(QObject *parent) : QThread(parent)
{

}

SDLThread::~SDLThread()
{
    if (this->isRunning())
    {
        this->StopLoop();
    }
}

void SDLThread::StopLoop(void)
{
    this->keepLoopRunning = false;
    while (this->isRunning())
    {
        // wait until we're not running anymore
    }
}

SDLThreadAction SDLThread::GetCurrentAction(void)
{
    return this->currentAction;
}

void SDLThread::SetAction(SDLThreadAction action)
{
    this->currentAction = action;
}

void SDLThread::run(void)
{
    while (this->keepLoopRunning)
    {
        switch (this->currentAction)
        {
            default:
            case SDLThreadAction::None:
                break;
            case SDLThreadAction::SDLPumpEvents:
            {
                SDL_PumpEvents();
            } break;
            case SDLThreadAction::GetInputDevices:
            {
                // force re-fresh joystick list
                SDL_UpdateJoysticks();

                QString name;
                QString path;
                QString serial;

                SDL_Gamepad* controller;
                SDL_Joystick* joystick;

                int numJoysticks = 0;
                SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);

                for (int i = 0; i < numJoysticks; i++)
                {
                    SDL_JoystickID joystickId = joysticks[i];

                    if (SDL_IsGamepad(joystickId))
                    {
                        controller = SDL_OpenGamepad(joystickId);
                        if (controller == nullptr)
                        { // skip invalid controllers
                            continue;
                        }
                        name = SDL_GetGamepadName(controller);
                        path = SDL_GetGamepadPath(controller);
                        serial = SDL_GetGamepadSerial(controller);
                        SDL_CloseGamepad(controller);
                    }
                    else
                    {
                        joystick = SDL_OpenJoystick(joystickId);
                        if (joystick == nullptr)
                        { // skip invalid joysticks
                            continue;
                        }
                        name = SDL_GetJoystickName(joystick);
                        path = SDL_GetJoystickPath(joystick);
                        serial = SDL_GetJoystickSerial(joystick);
                        SDL_CloseJoystick(joystick);
                    }

                    if (name != nullptr)
                    {
                        emit this->OnInputDeviceFound(name, path, serial, joystickId);
                    }
                }
                this->currentAction = SDLThreadAction::None;
                emit this->OnDeviceSearchFinished();
            } break;
        }

        // sleep for 10ms
        QThread::msleep(10);
    }
}
