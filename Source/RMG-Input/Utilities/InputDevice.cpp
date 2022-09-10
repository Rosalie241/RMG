/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "InputDevice.hpp"

using namespace Utilities;

InputDevice::InputDevice()
{

}

InputDevice::~InputDevice()
{
    this->CloseDevice();
}

void InputDevice::SetSDLThread(Thread::SDLThread* sdlThread)
{
    this->sdlThread = sdlThread;
    connect(this->sdlThread, &Thread::SDLThread::OnInputDeviceFound, this,
        &InputDevice::on_SDLThread_DeviceFound);
    connect(this->sdlThread, &Thread::SDLThread::OnDeviceSearchFinished, this,
        &InputDevice::on_SDLThread_DeviceSearchFinished);
}

SDL_Joystick* InputDevice::GetJoystickHandle()
{
    return this->joystick;
}

SDL_GameController* InputDevice::GetGameControllerHandle()
{
    return this->gameController;
}

bool InputDevice::StartRumble(void)
{
    if (this->gameController != nullptr)
    {
        return SDL_GameControllerRumble(this->gameController, 0xFFFF, 0xFFFF, SDL_HAPTIC_INFINITY) == 0;
    }
    else if (this->joystick != nullptr)
    {
        return SDL_JoystickRumble(this->joystick, 0xFFFF, 0xFFFF, SDL_HAPTIC_INFINITY) == 0;
    }

    return false;
}

bool InputDevice::StopRumble(void)
{
    if (this->gameController != nullptr)
    {
        return SDL_GameControllerRumble(this->gameController, 0, 0, 0) == 0;
    }
    else if (this->joystick != nullptr)
    {
        return SDL_JoystickRumble(this->joystick, 0, 0, 0) == 0;
    }

    return false;
}

bool InputDevice::IsAttached(void)
{
    return SDL_JoystickGetAttached(this->joystick) == SDL_TRUE;
}

bool InputDevice::HasOpenDevice()
{
    return this->hasOpenDevice;
}

void InputDevice::OpenDevice(std::string name, int num)
{
    // wait until SDLThread is done first
    while (this->sdlThread->GetCurrentAction() != SDLThreadAction::None)
    {
        QThread::msleep(50);
    }

    this->foundDevicesWithNameMatch.clear();
    this->desiredDeviceName = name;
    this->desiredDeviceNum = num;
    this->isOpeningDevice = true;

    // tell SDLThread to query input devices
    this->sdlThread->SetAction(SDLThreadAction::GetInputDevices);
}

bool InputDevice::IsOpeningDevice(void)
{
    return this->isOpeningDevice;
}

bool InputDevice::CloseDevice()
{
    if (this->joystick != nullptr)
    {
        SDL_JoystickClose(this->joystick);
        this->joystick = nullptr;
    }

    if (this->gameController != nullptr)
    {
        SDL_GameControllerClose(this->gameController);
        this->gameController = nullptr;
    }

    return true;
}

void InputDevice::on_SDLThread_DeviceFound(QString name, int number)
{
    if ((!this->isOpeningDevice) || 
        (this->desiredDeviceName != name.toStdString()))
    {
        return;
    }

    SDLDevice device;
    device.name = name.toStdString();
    device.number = number;

    this->foundDevicesWithNameMatch.push_back(device);
}

void InputDevice::on_SDLThread_DeviceSearchFinished(void)
{
    if (!this->isOpeningDevice)
    {
        return;
    }

    this->CloseDevice();

    if (this->foundDevicesWithNameMatch.empty())
    {
        this->isOpeningDevice = false;
        this->hasOpenDevice = false;
        return;
    }

    // try to find exact match
    SDLDevice device;
    device.name = this->desiredDeviceName;
    device.number = this->desiredDeviceNum;

    auto iter = std::find(this->foundDevicesWithNameMatch.begin(), this->foundDevicesWithNameMatch.end(), device);
    if (iter != this->foundDevicesWithNameMatch.end())
    { // use exact match
        device.name = iter->name;
        device.number = iter->number;
    }
    else
    { // no exact match, use first with name match
        device = this->foundDevicesWithNameMatch.at(0);
    }

    this->joystick = SDL_JoystickOpen(device.number);
    if (SDL_IsGameController(device.number))
    {
        this->gameController = SDL_GameControllerOpen(device.number);
    }

    this->isOpeningDevice = false;
    this->hasOpenDevice = this->joystick != nullptr || this->gameController == nullptr;
}
