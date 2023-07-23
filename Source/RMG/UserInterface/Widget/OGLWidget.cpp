/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "OGLWidget.hpp"

#include <QGuiApplication>
#include <RMG-Core/Core.hpp>

using namespace UserInterface::Widget;

OGLWidget::OGLWidget(QWidget *parent)
{
    // create window container
    this->widgetContainer = QWidget::createWindowContainer(this, parent);

    // on wayland we have to make sure that the widget
    // has a black background palette set, else
    // the window will have the theme as background color
    if (QGuiApplication::platformName() == "wayland")
    {
        QPalette blackPalette;
        blackPalette.setColor(QPalette::Window, Qt::black);
        this->widgetContainer->setAutoFillBackground(true); 
        this->widgetContainer->setPalette(blackPalette);
    }

    this->setSurfaceType(QWindow::OpenGLSurface);
    this->openGLcontext = new QOpenGLContext();
}

OGLWidget::~OGLWidget(void)
{
    this->openGLcontext->deleteLater();
}

void OGLWidget::MoveContextToThread(QThread* thread)
{
    this->GetContext()->doneCurrent();
    this->GetContext()->create();
    this->GetContext()->moveToThread(thread);
}

QOpenGLContext* OGLWidget::GetContext()
{
    return this->openGLcontext;
}

void OGLWidget::SetHideCursor(bool hide)
{
    this->setCursor(hide ? Qt::BlankCursor : Qt::ArrowCursor);
}

QWidget *OGLWidget::GetWidget(void)
{
    return this->widgetContainer;
}

void OGLWidget::resizeEvent(QResizeEvent *event)
{
    if (this->timerId != 0)
    {
        this->killTimer(this->timerId);
        this->timerId = 0;
    }

    this->timerId = this->startTimer(100);

    // account for HiDPI scaling
    // see https://github.com/Rosalie241/RMG/issues/2
    this->width = event->size().width() * this->devicePixelRatio();
    this->height = event->size().height() * this->devicePixelRatio();
}

void OGLWidget::timerEvent(QTimerEvent *event)
{
    // only remove current timer
    // when setting the video size succeeds
    if (CoreSetVideoSize(this->width, this->height))
    {
        this->killTimer(this->timerId);
        this->timerId = 0;
    }
}
