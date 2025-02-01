/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Callbacks.hpp"

#include <RMG-Core/Settings.hpp>

#include <QTimerEvent>
#include <QMutex>


//
// Local Variables
//

static CoreCallbacks* l_CoreCallbacks = nullptr;
static bool           l_showVerboseMessages = false;

static QMutex                     l_CallbackMessagesMutex;
static QList<CoreCallbackMessage> l_CallbackMessages;

//
// Exported Functions
//

CoreCallbacks::CoreCallbacks(QObject* parent) : QObject(parent)
{

}

CoreCallbacks::~CoreCallbacks()
{
    l_CoreCallbacks = nullptr;
}

bool CoreCallbacks::Init(void)
{
    // needed for Qt
    qRegisterMetaType<CoreDebugMessageType>("CoreDebugMessageType");
    qRegisterMetaType<CoreDebugMessageType>("CoreStateCallbackType");
    qRegisterMetaType<CoreCallbackMessage>("CoreCallbackMessage");

    this->LoadSettings();

    l_CoreCallbacks = this;
    
    if (!CoreSetupCallbacks(this->coreDebugCallback, this->coreStateCallback))
    {
        return false;
    }

    this->callbackTimerId = this->startTimer(100);
    return true;
}

void CoreCallbacks::Stop(void)
{
    l_CoreCallbacks = nullptr;

    this->killTimer(this->callbackTimerId);
    this->callbackTimerId = -1;
}

void CoreCallbacks::LoadSettings(void)
{
    l_showVerboseMessages = CoreSettingsGetBoolValue(SettingsID::GUI_ShowVerboseLogMessages);
}

void CoreCallbacks::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == this->callbackTimerId)
    {
        l_CallbackMessagesMutex.lock();
        if (!l_CallbackMessages.isEmpty())
        {
            emit this->OnCoreDebugCallback(l_CallbackMessages);
            l_CallbackMessages.clear();
        }
        l_CallbackMessagesMutex.unlock();
    }
}

void CoreCallbacks::coreDebugCallback(CoreDebugMessageType type, std::string context, std::string message)
{
    if (l_CoreCallbacks == nullptr)
    {
        return;
    }

    // only show verbose messages when enabled
    if (!l_showVerboseMessages && type == CoreDebugMessageType::Verbose)
    {
        return;
    }

    l_CallbackMessagesMutex.lock();
    l_CallbackMessages.append({type, QString::fromStdString(context), QString::fromStdString(message)});
    l_CallbackMessagesMutex.unlock();
}

void CoreCallbacks::coreStateCallback(CoreStateCallbackType type, int value)
{
    if (l_CoreCallbacks == nullptr)
    {
        return;
    }

    emit l_CoreCallbacks->OnCoreStateCallback(type, value);
}
