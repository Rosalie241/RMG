/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SDLTHREAD_HPP
#define SDLTHREAD_HPP

#include <QThread>

enum class SDLThreadAction
{
    None = 0,
    SDLPumpEvents,
    GetInputDevices,
};

namespace Thread
{
class SDLThread : public QThread
{
    Q_OBJECT
public:
    SDLThread(QObject *);
    ~SDLThread(void);

    void run(void) override;

    void StopLoop(void);

    SDLThreadAction GetCurrentAction(void);
    void SetAction(SDLThreadAction action);

private:
    bool keepLoopRunning = true;
    SDLThreadAction currentAction = SDLThreadAction::None;

signals:
    void OnInputDeviceFound(QString, int);
    void OnDeviceSearchFinished(void);
};
} // namespace Thread

#endif // SDLTHREAD_HPP
