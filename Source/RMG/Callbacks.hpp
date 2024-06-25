/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef RMG_CALLBACKS_HPP
#define RMG_CALLBACKS_HPP

#include <QList>
#include <QObject>

#include <RMG-Core/Core.hpp>

#include <string>

struct CoreCallbackMessage
{
    CoreDebugMessageType Type;
    QString              Context;
    QString              Message;
};

class CoreCallbacks : public QObject
{
    Q_OBJECT

public:
    CoreCallbacks(QObject* parent);
    ~CoreCallbacks();

    bool Init(void);
    void Stop(void);

    void LoadSettings(void);

protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:
    static void coreDebugCallback(CoreDebugMessageType type, std::string context, std::string message);
    static void coreStateCallback(CoreStateCallbackType type, int value);


    int callbackTimerId = -1;

signals:
    void OnCoreDebugCallback(QList<CoreCallbackMessage> callbackMessages);
    void OnCoreStateCallback(CoreStateCallbackType type, int value);
};

#endif // RMG_CALLBACKS_HPP
