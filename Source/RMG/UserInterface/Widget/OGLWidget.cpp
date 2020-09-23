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

void OGLWidget::SetThread(QThread* thread)
{
    this->doneCurrent();
    this->context()->moveToThread(thread);
}

QWidget* OGLWidget::GetWidget(void)
{
    return QWidget::createWindowContainer(this);
}


void OGLWidget::exposeEvent(QExposeEvent *)
{
}

