/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
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
        if (((QKeyEvent *)event)->key() != Qt::Key_Escape)
        {
            emit this->on_EventFilter_KeyPressed((QKeyEvent *)event);
            return true;
        }
    } break;
    case QEvent::Type::KeyRelease:
    {
        if (((QKeyEvent *)event)->key() != Qt::Key_Escape)
        {
            emit this->on_EventFilter_KeyReleased((QKeyEvent *)event);
            return true;
        }
    } break;
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}
