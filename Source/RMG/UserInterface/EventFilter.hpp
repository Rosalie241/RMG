/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef EVENTFILTER_HPP
#define EVENTFILTER_HPP

#include <QFocusEvent>
#include <QKeyEvent>
#include <QList>
#include <QObject>

namespace UserInterface
{
class EventFilter : public QObject
{
    Q_OBJECT

  public:
    EventFilter(QObject *);
    ~EventFilter(void);

  protected:
    bool eventFilter(QObject *, QEvent *);

  signals:
    void on_EventFilter_KeyPressed(QKeyEvent *);
    void on_EventFilter_KeyReleased(QKeyEvent *);

    void on_EventFilter_FileDropped(QDropEvent *);
};
} // namespace UserInterface

#endif // EVENTFILTER_HPP