#include "OGLWidget.hpp"
#include "../../Globals.hpp"

using namespace UserInterface::Widget;

#include <iostream>

OGLWidget::OGLWidget(void)
{
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

QWidget *OGLWidget::GetWidget(void)
{
    return QWidget::createWindowContainer(this);
}

void OGLWidget::exposeEvent(QExposeEvent *)
{
}
