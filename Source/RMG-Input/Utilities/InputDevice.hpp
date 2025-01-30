/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef INPUTDEVICE_HPP
#define INPUTDEVICE_HPP

#include "common.hpp"

#include <QObject>
#include <string>
#include <SDL.h>

#include "Thread/SDLThread.hpp"

namespace Utilities
{
class InputDevice : public QObject
{
Q_OBJECT
public:
    InputDevice();
    ~InputDevice();

    void SetSDLThread(Thread::SDLThread* sdlThread);

    SDL_Joystick*       GetJoystickHandle(void);
    SDL_GameController* GetGameControllerHandle(void);

    bool StartRumble(void);
    bool StopRumble(void);

    // returns whether the device is attached
    bool IsAttached(void);

    // returns whether a device has been opened
    bool HasOpenDevice(void);

    // tries to open device with given name, path, serial & num
    void OpenDevice(std::string name, std::string path, std::string serial, int num);

    // returns whether we're still trying to open the device
    bool IsOpeningDevice(void);

    // tries to close opened device
    bool CloseDevice(void);

private:
    SDL_Joystick*       joystick = nullptr;
    SDL_GameController* gameController = nullptr;

    bool hasOpenDevice = false;
    bool isOpeningDevice = false;

    Thread::SDLThread* sdlThread = nullptr;

    SDLDevice desiredDevice;
    std::vector<SDLDevice> foundDevicesWithNameMatch;

private slots:
    void on_SDLThread_DeviceFound(QString name, QString path, QString serial, int number);
    void on_SDLThread_DeviceSearchFinished(void);
};
} // namespace Utilities

#endif // INPUTDEVICE_HPP