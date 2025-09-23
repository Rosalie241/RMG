/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAINDIALOG_HPP
#define MAINDIALOG_HPP

#include "common.hpp"
#include <QDialog>

#include "Widget/ControllerWidget.hpp"
#include "Thread/SDLThread.hpp"
#include "EventFilter.hpp"

#include "ui_MainDialog.h"

#include <RMG-Core/RomSettings.hpp>
#include <RMG-Core/RomHeader.hpp>

namespace UserInterface
{
class MainDialog : public QDialog, private Ui::MainDialog
{
Q_OBJECT
private:
    QTimer* inputPollTimer;
    Thread::SDLThread* sdlThread;

    QList<InputDevice> oldInputDeviceList;
    QList<InputDevice> inputDeviceList;
    bool updatingDeviceList = false;

    QList<Widget::ControllerWidget*> controllerWidgets;
    SDL_Joystick* currentJoystick         = nullptr;
    SDL_Gamepad* currentController = nullptr;
    InputDevice currentDevice;

    int previousTabWidgetIndex = 0;

    EventFilter* eventFilter;

    void addInputDevice(const InputDevice& device);
    void removeInputDevice(const InputDevice& device);

    void openInputDevice(InputDevice device);
    void closeInputDevice();

public:
    MainDialog(QWidget *parent, Thread::SDLThread*, bool, CoreRomHeader, CoreRomSettings);
    ~MainDialog(void);

public slots:
    void on_InputPollTimer_triggered();

    void on_ControllerWidget_CurrentInputDeviceChanged(ControllerWidget*, InputDevice);
    void on_ControllerWidget_RefreshInputDevicesButtonClicked();

    void on_ControllerWidget_UserProfileAdded(QString, QString);
    void on_ControllerWidget_UserProfileRemoved(QString, QString);

    void on_tabWidget_currentChanged(int);

    void on_SDLThread_DeviceFound(QString, QString, QString, SDL_JoystickID);
    void on_SDLThread_DeviceSearchFinished(void);

private slots:
    void on_EventFilter_KeyPressed(QKeyEvent *);
    void on_EventFilter_KeyReleased(QKeyEvent *);

    void accept(void) Q_DECL_OVERRIDE;
    void reject(void) Q_DECL_OVERRIDE;
};
}

#endif // MAINDIALOG_HPP
