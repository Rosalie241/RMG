#include "OGLWidget.hpp"

using namespace UserInterface::Widget;

#include <iostream>
#include <RMG-Core/Core.hpp>

OGLWidget::OGLWidget(QWidget *parent)
{
    this->parent = parent;
    this->timerId = 0;
}

OGLWidget::~OGLWidget(void)
{
}

void OGLWidget::MoveToThread(QThread *thread)
{
    this->doneCurrent();

    this->context()->create();

    this->context()->moveToThread(thread);
}

void OGLWidget::SetAllowResizing(bool value)
{
    this->allowResizing = value;
}

void OGLWidget::SetHideCursor(bool value)
{
    this->setCursor(value ? Qt::BlankCursor : Qt::ArrowCursor);
}

QWidget *OGLWidget::GetWidget(void)
{
    QWidget *widget = QWidget::createWindowContainer(this);
    widget->setParent(this->parent);
    return widget;
}

void OGLWidget::exposeEvent(QExposeEvent *)
{
}

void OGLWidget::resizeEvent(QResizeEvent *event)
{
    QOpenGLWindow::resizeEvent(event);

    if (!this->allowResizing)
        return;

    if (this->timerId != 0)
    {
        this->killTimer(this->timerId);
        this->timerId = 0;
    }

    this->timerId = this->startTimer(100);

    // account for HiDPI scaling
    // see https://github.com/Rosalie241/RMG/issues/2
    this->width = event->size().width() * this->devicePixelRatioF();
    this->height = event->size().height() * this->devicePixelRatioF();
}

void OGLWidget::timerEvent(QTimerEvent *event)
{
    CoreSetVideoSize(this->width, this->height);

    // remove current timer
    this->killTimer(this->timerId);
    this->timerId = 0;
    this->requestActivate();
}
