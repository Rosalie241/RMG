/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "EventFilter.hpp"

using namespace UserInterface;

EventFilter::EventFilter(QObject *parent) : QObject(parent)
{
}

EventFilter::~EventFilter(void)
{
}

bool EventFilter::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::Type::KeyPress:
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() != Qt::Key_Escape)
        {
            emit this->on_EventFilter_KeyPressed(keyEvent);
            return true;
        }
    } break;
    case QEvent::Type::KeyRelease:
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() != Qt::Key_Escape)
        {
            emit this->on_EventFilter_KeyReleased(keyEvent);
            return true;
        }
    } break;
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}
;