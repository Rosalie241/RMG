/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef HOTKEYSTHREAD_HPP
#define HOTKEYSTHREAD_HPP

#include <QThread>

enum class HotkeysThreadState
{
    RomOpened,
    RomClosed,
};

namespace Thread
{
class HotkeysThread : public QThread
{
    Q_OBJECT
public:
    HotkeysThread(std::function<void(int)> checkHotkeysFunc, QObject *parent);
    ~HotkeysThread(void);

    void run(void) override;

    void SetState(HotkeysThreadState state);

    void StopLoop(void);

private:
    bool keepLoopRunning = true;
    std::function<void(int)> checkHotkeysFunc;
    HotkeysThreadState state = HotkeysThreadState::RomClosed;

};
} // namespace Thread

#endif // HOTKEYSTHREAD_HPP
