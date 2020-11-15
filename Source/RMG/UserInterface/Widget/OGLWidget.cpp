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

QWidget *OGLWidget::GetWidget(void)
{
    QWidget *widget = QWidget::createWindowContainer(this);
    widget->setParent(this->parent);
    return widget;
}

void OGLWidget::exposeEvent(QExposeEvent *)
{
}
