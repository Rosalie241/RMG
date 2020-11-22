#include "OGLWidget.hpp"
#include "../../Globals.hpp"

using namespace UserInterface::Widget;

#include <iostream>

OGLWidget::OGLWidget(QWidget *parent)
{
    this->parent = parent;
}

OGLWidget::~OGLWidget(void)
{
}

void OGLWidget::SetThread(QThread *thread)
{
    this->doneCurrent();

    // Does this leak?
    // seems to be needed for GFX plugin switching
    // otherwise we'll get some funky colors
    this->context()->create();

    this->context()->moveToThread(thread);
}

void OGLWidget::SetAllowResizing(bool allow)
{
    this->allowResizing = allow;
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
    this->width = event->size().width() * this->devicePixelRatio();
    this->height = event->size().height() * this->devicePixelRatio();
}

void OGLWidget::timerEvent(QTimerEvent *event)
{
    g_MupenApi.Core.SetVideoSize(this->width, this->height);

    // remove current timer
    this->killTimer(this->timerId);
    this->timerId = 0;
    this->requestActivate();
}
