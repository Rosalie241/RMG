/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MainDialog.hpp"
#include "Widget/ControllerWidget.hpp"
#include "Utilities/QtKeyToSdl2Key.hpp"

#include <RMG-Core/Core.hpp>

#include <SDL.h>
#include <QTimer>

using namespace UserInterface;

MainDialog::MainDialog(QWidget* parent, Thread::SDLThread* sdlThread) : QDialog(parent)
{
    this->setupUi(this);
    this->setWindowIcon(QIcon(":Resource/RMG.png"));

    // setup SDL thread
    this->sdlThread = sdlThread;
    connect(this->sdlThread, &Thread::SDLThread::OnInputDeviceFound, this,
        &MainDialog::on_SDLThread_DeviceFound);
    connect(this->sdlThread, &Thread::SDLThread::OnDeviceSearchFinished, this,
        &MainDialog::on_SDLThread_DeviceSearchFinished);

    // setup EventFilter
    this->eventFilter = new EventFilter(this);
    connect(this->eventFilter, &EventFilter::on_EventFilter_KeyPressed, this,
            &MainDialog::on_EventFilter_KeyPressed);
    connect(this->eventFilter, &EventFilter::on_EventFilter_KeyReleased, this,
            &MainDialog::on_EventFilter_KeyReleased);

    // each tab needs its own ControllerWidget
    for (int i = 0; i < this->tabWidget->count(); i++)
    {
        Widget::ControllerWidget* widget = new Widget::ControllerWidget(this, this->eventFilter);
        widget->SetSettingsSection("Rosalie's Mupen GUI - Input Plugin Profile " + QString::number(i));
        widget->LoadSettings();
        this->tabWidget->widget(i)->layout()->addWidget(widget);
        controllerWidgets.push_back(widget);
        connect(widget, &Widget::ControllerWidget::CurrentInputDeviceChanged, this,
            &MainDialog::on_ControllerWidget_CurrentInputDeviceChanged);
        connect(widget, &Widget::ControllerWidget::RefreshInputDevicesButtonClicked, this,
            &MainDialog::on_ControllerWidget_RefreshInputDevicesButtonClicked);
    }

    // always add keyboard device
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->AddInputDevice("Keyboard", -1);
    }

    // fill device list at least once
    this->on_ControllerWidget_RefreshInputDevicesButtonClicked();

    this->inputPollTimer = new QTimer(this);
    connect(this->inputPollTimer, &QTimer::timeout, this, &MainDialog::on_InputPollTimer_triggered);
    this->inputPollTimer->start(50);
}

MainDialog::~MainDialog()
{
    this->closeInputDevice();
}

void MainDialog::addInputDevice(QString deviceName, int deviceNum)
{
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->AddInputDevice(deviceName, deviceNum);
    }
}

void MainDialog::removeInputDevice(QString deviceName, int deviceNum)
{
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->RemoveInputDevice(deviceName, deviceNum);
    }
}

void MainDialog::openInputDevice(QString deviceName, int deviceNum)
{
    SDL_JoystickID joystickId;
    Widget::ControllerWidget* controllerWidget;
    controllerWidget = controllerWidgets.at(this->tabWidget->currentIndex());

    // we don't need to open a keyboard
    if (deviceNum == -1)
    {
        currentDeviceName = "";
        currentDeviceNum = -1;
        controllerWidget->SetCurrentJoystickID(-1);
        return;
    }

    if (SDL_IsGameController(deviceNum) == SDL_TRUE)
    {
        currentJoystick = nullptr;
        currentController = SDL_GameControllerOpen(deviceNum);
    }
    else
    {
        currentJoystick = SDL_JoystickOpen(deviceNum);
        currentController = nullptr;
    }

    currentDeviceNum = deviceNum;
    joystickId = SDL_JoystickGetDeviceInstanceID(deviceNum);
    controllerWidget->SetCurrentJoystickID(joystickId);
    controllerWidget->SetIsCurrentJoystickGameController(currentController != nullptr);
}

void MainDialog::closeInputDevice()
{
    if (this->currentJoystick != nullptr)
    {
        SDL_JoystickClose(this->currentJoystick);
        this->currentJoystick = nullptr;
    }

    if (this->currentController != nullptr)
    {
        SDL_GameControllerClose(this->currentController);
        this->currentController = nullptr;
    }
}

void MainDialog::on_InputPollTimer_triggered()
{
    Widget::ControllerWidget* controllerWidget;
    controllerWidget = controllerWidgets.at(this->tabWidget->currentIndex());

    // if the current controller widget
    // is disabled (by the user), 
    // we don't need to do anything
    if (!controllerWidget->IsPluggedIn())
    {
        this->sdlThread->SetAction(SDLThreadAction::None);
        return;
    }

    // verify/update the SDL thread action state
    switch (this->sdlThread->GetCurrentAction())
    {
        case SDLThreadAction::GetInputDevices:
            // don't do anything when we're querying the input devices
            return;
        case SDLThreadAction::None:
            // when the SDL thread is doing nothing, pump the sdl events instead
            sdlThread->SetAction(SDLThreadAction::SDLPumpEvents);
            break;
        default:
            break;
    }

    // check if controller has been disconnected,
    // if so, keep trying to re-open it
    if ((this->currentJoystick != nullptr && !SDL_JoystickGetAttached(this->currentJoystick)) ||
        (this->currentController != nullptr && !SDL_GameControllerGetAttached(this->currentController)))
    {
        this->closeInputDevice();
        this->openInputDevice(this->currentDeviceName, this->currentDeviceNum);
    }

    // process SDL events
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, 0, SDL_LASTEVENT) == 1)
    {
        controllerWidget->on_MainDialog_SdlEvent(&event);
    }

    controllerWidget->on_MainDialog_SdlEventPollFinished();
}

void MainDialog::on_ControllerWidget_CurrentInputDeviceChanged(ControllerWidget* widget, QString deviceName, int deviceNum)
{
    Widget::ControllerWidget* currentWidget;
    currentWidget = controllerWidgets.at(this->tabWidget->currentIndex());

    // ignore input device changes
    // when it's not the current controllerwidget
    if (widget != currentWidget)
    {
        return;
    }

    this->closeInputDevice();

    // only open device,
    // when it's not a keyboard
    if (deviceNum != -1)
    {
        this->openInputDevice(deviceName, deviceNum);
    }
}

void MainDialog::on_ControllerWidget_RefreshInputDevicesButtonClicked()
{
    if (this->updatingDeviceList)
    {
        return;
    }

    this->updatingDeviceList = true;
    this->inputDeviceList.clear();
    this->sdlThread->SetAction(SDLThreadAction::GetInputDevices);
}

void MainDialog::on_tabWidget_currentChanged(int index)
{
    QString deviceName;
    int deviceNum;

    Widget::ControllerWidget* controllerWidget = controllerWidgets.at(index);
    controllerWidget->ClearControllerImage();

    this->closeInputDevice();

    controllerWidget->GetCurrentInputDevice(deviceName, deviceNum);

    // only open device,
    // when it's not a keyboard
    if (deviceNum != -1)
    {
        this->openInputDevice(deviceName, deviceNum);
    }
}

void MainDialog::on_SDLThread_DeviceFound(QString deviceName, int deviceNum)
{
    inputDevice_t inputDevice = {deviceName, deviceNum};
    this->inputDeviceList.append(inputDevice);
}

void MainDialog::on_SDLThread_DeviceSearchFinished(void)
{
    // compare inputDeviceList with oldInputDeviceList
    // and signal the changes
    for (auto& inputDevice : this->inputDeviceList)
    {
        if (!this->oldInputDeviceList.contains(inputDevice))
        {
            this->addInputDevice(inputDevice.deviceName, inputDevice.deviceNum);
        }
    }
    for (auto& inputDevice : this->oldInputDeviceList)
    {
        if (!this->inputDeviceList.contains(inputDevice))
        {
            this->removeInputDevice(inputDevice.deviceName, inputDevice.deviceNum);
        }
    }

    // update old device list
    this->oldInputDeviceList = this->inputDeviceList;

    // we can refresh safely again
    this->updatingDeviceList = false;

    // device search has been completed,
    // signal that each widget can check device settings
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->CheckInputDeviceSettings();
    }
}

void MainDialog::on_EventFilter_KeyPressed(QKeyEvent *event)
{
    int key = Utilities::QtKeyToSdl2Key(event->key());
    int mod = Utilities::QtModKeyToSdl2ModKey(event->modifiers());

    SDL_KeyboardEvent keyboardEvent;
    keyboardEvent.state = SDL_PRESSED;
    keyboardEvent.type = SDL_KEYDOWN;
    keyboardEvent.keysym.scancode = (SDL_Scancode)key;
    keyboardEvent.keysym.sym = (SDL_Keycode)key;
    keyboardEvent.keysym.mod = mod;

    SDL_Event sdlEvent;
    sdlEvent.key = keyboardEvent;
    sdlEvent.type = SDL_KEYDOWN;

    SDL_PeepEvents(&sdlEvent, 1, SDL_ADDEVENT, 0, 0);
}

void MainDialog::on_EventFilter_KeyReleased(QKeyEvent *event)
{
    int key = Utilities::QtKeyToSdl2Key(event->key());
    int mod = Utilities::QtModKeyToSdl2ModKey(event->modifiers());

    SDL_KeyboardEvent keyboardEvent;
    keyboardEvent.state = SDL_RELEASED;
    keyboardEvent.type = SDL_KEYUP;
    keyboardEvent.keysym.scancode = (SDL_Scancode)key;
    keyboardEvent.keysym.sym = (SDL_Keycode)key;
    keyboardEvent.keysym.mod = mod;

    SDL_Event sdlEvent;
    sdlEvent.key = keyboardEvent;
    sdlEvent.type = SDL_KEYUP;

    SDL_PeepEvents(&sdlEvent, 1, SDL_ADDEVENT, 0, 0);
}

void MainDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);

    if (pushButton == okButton)
    {
        for (auto& controllerWidget : this->controllerWidgets)
        {
            controllerWidget->SaveSettings();
        }

        CoreSettingsSave();
    }
}

