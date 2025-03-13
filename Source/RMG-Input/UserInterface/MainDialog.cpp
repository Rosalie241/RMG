/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
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

Q_DECLARE_METATYPE(SDLDevice);

using namespace UserInterface;

MainDialog::MainDialog(QWidget* parent, Thread::SDLThread* sdlThread, bool romConfig, CoreRomHeader romHeader, CoreRomSettings romSettings) : QDialog(parent)
{
    qRegisterMetaType<SDLDevice>();
    
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
        widget->SetOnlyLoadGameProfile(romConfig, romHeader, romSettings);
        widget->SetSettingsSection("Player " + QString::number(i + 1), "Rosalie's Mupen GUI - Input Plugin Profile " + QString::number(i));
        widget->LoadSettings();
        this->tabWidget->widget(i)->layout()->addWidget(widget);
        controllerWidgets.push_back(widget);
        connect(widget, &Widget::ControllerWidget::CurrentInputDeviceChanged, this,
            &MainDialog::on_ControllerWidget_CurrentInputDeviceChanged);
        connect(widget, &Widget::ControllerWidget::RefreshInputDevicesButtonClicked, this,
            &MainDialog::on_ControllerWidget_RefreshInputDevicesButtonClicked);
        connect(widget, &Widget::ControllerWidget::UserProfileAdded, this,
            &MainDialog::on_ControllerWidget_UserProfileAdded);
        connect(widget, &Widget::ControllerWidget::UserProfileRemoved, this,
            &MainDialog::on_ControllerWidget_UserProfileRemoved);
    }

    // always add keyboard device
    for (auto& controllerWidget : this->controllerWidgets)
    {
#ifdef VRU
        // both games which support the VRU,
        // require the VRU to be in port #4,
        // so we only have to expose it there
        if (controllerWidget == this->controllerWidgets.last())
        {
            controllerWidget->AddInputDevice({"Voice Recognition Unit", "", "", static_cast<int>(InputDeviceType::EmulateVRU)});
        }
#endif // VRU
        controllerWidget->AddInputDevice({"None", "", "", static_cast<int>(InputDeviceType::None)});
        controllerWidget->AddInputDevice({"Automatic", "", "", static_cast<int>(InputDeviceType::Automatic)});
        controllerWidget->AddInputDevice({"Keyboard", "", "", static_cast<int>(InputDeviceType::Keyboard)});
        controllerWidget->SetInitialized(true);
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

void MainDialog::addInputDevice(SDLDevice device)
{
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->AddInputDevice(device);
    }
}

void MainDialog::removeInputDevice(SDLDevice device)
{
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->RemoveInputDevice(device);
    }
}

void MainDialog::openInputDevice(SDLDevice device)
{
    SDL_JoystickID joystickId;
    Widget::ControllerWidget* controllerWidget;
    controllerWidget = this->controllerWidgets.at(this->tabWidget->currentIndex());

    // we don't need to open a keyboard or VRU
    if (device.number == static_cast<int>(InputDeviceType::None) ||
        device.number == static_cast<int>(InputDeviceType::Keyboard) ||
        device.number == static_cast<int>(InputDeviceType::EmulateVRU))
    {
        this->currentDevice = { "", "", "", device.number };
        controllerWidget->SetCurrentJoystickID(this->currentDevice.number);
        controllerWidget->SetCurrentJoystick(nullptr, nullptr);
        return;
    }

    // handle automatic mode
    if (device.number == static_cast<int>(InputDeviceType::Automatic))
    {
        int currentIndex = this->tabWidget->currentIndex();
        if (currentIndex < this->inputDeviceList.size())
        { // use device when there's one
            device.number = this->inputDeviceList.at(currentIndex).number;
        }
        else
        { // no device found, fallback to keyboard
            this->currentDevice = { "", "", "", static_cast<int>(InputDeviceType::Keyboard) };
            controllerWidget->SetCurrentJoystickID(this->currentDevice.number);
            controllerWidget->SetCurrentJoystick(nullptr, nullptr);
            return;
        }
    }

    int controllerMode = CoreSettingsGetIntValue(SettingsID::Input_ControllerMode);
    if ((controllerMode == 0 && SDL_IsGameController(device.number) == SDL_TRUE) ||
        (controllerMode == 2))
    {
        this->currentJoystick = nullptr;
        this->currentController = SDL_GameControllerOpen(device.number);
    }
    else if (controllerMode == 0 || controllerMode == 1)
    {
        this->currentJoystick = SDL_JoystickOpen(device.number);
        this->currentController = nullptr;
    }

    this->currentDevice = device;
    joystickId = SDL_JoystickGetDeviceInstanceID(device.number);
    controllerWidget->SetCurrentJoystickID(joystickId);
    controllerWidget->SetIsCurrentJoystickGameController(currentController != nullptr);
    controllerWidget->SetCurrentJoystick(this->currentJoystick, this->currentController);
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
        this->openInputDevice(this->currentDevice);
    }

    // process SDL events
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, 0, SDL_LASTEVENT) == 1)
    {
        controllerWidget->on_MainDialog_SdlEvent(&event);
    }

    controllerWidget->on_MainDialog_SdlEventPollFinished();
}

void MainDialog::on_ControllerWidget_CurrentInputDeviceChanged(ControllerWidget* widget, SDLDevice device)
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

    // only open device when needed
    if (device.number != static_cast<int>(InputDeviceType::None) &&
        device.number != static_cast<int>(InputDeviceType::Keyboard) &&
        device.number != static_cast<int>(InputDeviceType::EmulateVRU))
    {
        this->openInputDevice(device);
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

void MainDialog::on_ControllerWidget_UserProfileAdded(QString name, QString section)
{
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->AddUserProfile(name, section);
    }
}

void MainDialog::on_ControllerWidget_UserProfileRemoved(QString name, QString section)
{
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->RemoveUserProfile(name, section);
    }
}

void MainDialog::on_tabWidget_currentChanged(int index)
{
    SDLDevice device;
    Widget::ControllerWidget* controllerWidget;

    // save previous tab's user profile
    if (previousTabWidgetIndex != -1 &&
        previousTabWidgetIndex != index)
    {
        controllerWidget = controllerWidgets.at(previousTabWidgetIndex);
        controllerWidget->SaveUserProfileSettings();
    }

    // update previous index
    previousTabWidgetIndex = index;

    // prepare switched tab
    controllerWidget = controllerWidgets.at(index);
    controllerWidget->ClearControllerImage();

    this->closeInputDevice();

    // re-load user profile settings
    controllerWidget->LoadUserProfileSettings();

    // retrieve current input device
    controllerWidget->GetCurrentInputDevice(device);

    // only open device when needed
    if (device.number != static_cast<int>(InputDeviceType::None) &&
        device.number != static_cast<int>(InputDeviceType::Keyboard) &&
        device.number != static_cast<int>(InputDeviceType::EmulateVRU))
    {
        this->openInputDevice(device);
    }
}

void MainDialog::on_SDLThread_DeviceFound(QString name, QString path, QString serial, int number)
{
    SDLDevice inputDevice = {name.toStdString(), path.toStdString(), serial.toStdString(), number};
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
            this->addInputDevice(inputDevice);
        }
    }
    for (auto& inputDevice : this->oldInputDeviceList)
    {
        if (!this->inputDeviceList.contains(inputDevice))
        {
            this->removeInputDevice(inputDevice);
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

void MainDialog::accept(void)
{
    Widget::ControllerWidget* controllerWidget;
    int currentIndex = this->tabWidget->currentIndex();

    for (int i = 0; i < this->controllerWidgets.count(); i++)
    {
        if (i != currentIndex)
        {
            controllerWidget = this->controllerWidgets.at(i);
            controllerWidget->SaveSettings();
        }
    }

    // the widget which has focus should be saved last
    controllerWidget = this->controllerWidgets.at(currentIndex);
    controllerWidget->SaveSettings();

    CoreSettingsSave();

    QDialog::accept();
}

void MainDialog::reject(void)
{
    for (auto& controllerWidget : this->controllerWidgets)
    {
        controllerWidget->RevertSettings();
    }

    CoreSettingsSave();

    QDialog::reject();
}
