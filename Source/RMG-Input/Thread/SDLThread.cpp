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
#include "main.hpp"

#include <RMG-Core/m64p/api/m64p_types.h>
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
                QString errorMessage;

                SDL_Gamepad* controller;
                SDL_Joystick* joystick;

                int joysticksCount = 0;
                SDL_JoystickID* joysticks = SDL_GetJoysticks(&joysticksCount);
                if (joysticks == nullptr)
                {
                    errorMessage = "SDLThread::run() SDL_GetJoysticks Failed: ";
                    errorMessage += SDL_GetError();
                    PluginDebugMessage(M64MSG_ERROR, errorMessage.toStdString());

                    // ensure count is reset
                    joysticksCount = 0;
                }


                for (int i = 0; i < joysticksCount; i++)
                {
                    SDL_JoystickID joystickId = joysticks[i];

                    if (SDL_IsGamepad(joystickId))
                    {
                        controller = SDL_OpenGamepad(joystickId);
                        if (controller == nullptr)
                        { // skip invalid controllers
                            errorMessage = "SDLThread::run(): SDL_OpenGamepad Failed: ";
                            errorMessage += SDL_GetError();
                            PluginDebugMessage(M64MSG_ERROR, errorMessage.toStdString());
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
                            errorMessage = "SDLThread::run(): SDL_OpenJoystick Failed: ";
                            errorMessage += SDL_GetError();
                            PluginDebugMessage(M64MSG_ERROR, errorMessage.toStdString());
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

                if (joysticks != nullptr)
                {
                    SDL_free(joysticks);
                }
            } break;
        }

        // sleep for 10ms
        QThread::msleep(10);
    }
}
