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

#include <QObject>

#include <RMG-Core/Core.hpp>

#include <string>

class CoreCallbacks : public QObject
{
    Q_OBJECT

public:
    CoreCallbacks(QObject* parent);
    ~CoreCallbacks();

    bool Init(void);
    void Stop(void);

    void LoadSettings(void);

private:
    static void coreDebugCallback(CoreDebugMessageType type, std::string context, std::string message);

signals:
    void OnCoreDebugCallback(CoreDebugMessageType type, QString context, QString message);
};

#endif // RMG_CALLBACKS_HPP
