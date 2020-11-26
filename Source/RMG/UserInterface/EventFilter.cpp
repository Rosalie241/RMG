#include "EventFilter.hpp"
#include <qcoreevent.h>
#include <qobject.h>

using namespace UserInterface;

EventFilter::EventFilter(QObject *parent) : QObject(parent)
{
}

EventFilter::~EventFilter(void)
{
}

bool EventFilter::eventFilter(QObject *object, QEvent *event)
{
    static QObject *lastFocusObject = nullptr;

    switch (event->type())
    {
    case QEvent::Type::KeyPress:
        emit this->on_EventFilter_KeyPressed((QKeyEvent *)event);
        return true;
    case QEvent::Type::Drop:
        emit this->on_EventFilter_FileDropped((QDropEvent *)event);
        return true;
    case QEvent::Type::KeyRelease:
        emit this->on_EventFilter_KeyReleased((QKeyEvent *)event);
        return true;
    // it seems like Qt loses focus whenever you click on the menubar,
    // even when re-implementing QMenuBar's focusIn/Out, it wont work correctly.
    /*case QEvent::Type::FocusIn:
        lastFocusObject = object;
        std::cout << object << std::endl;
        emit this->on_EventFilter_FocusIn((QFocusEvent *)event);
        return true;
    case QEvent::Type::FocusOut:
        if (lastFocusObject == object)
            emit this->on_EventFilter_FocusOut((QFocusEvent *)event);
        return true;*/
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}
