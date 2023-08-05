/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "VKWidget.hpp"

#include <QGuiApplication>
#include <RMG-Core/Core.hpp>

using namespace UserInterface::Widget;

VKWidget::VKWidget(QWidget *parent)
{
    this->widgetContainer = QWidget::createWindowContainer(this, parent);
    this->setSurfaceType(QWindow::VulkanSurface);
}

VKWidget::~VKWidget(void)
{
}

void VKWidget::SetHideCursor(bool hide)
{
    this->setCursor(hide ? Qt::BlankCursor : Qt::ArrowCursor);
}

QWidget* VKWidget::GetWidget(void)
{
    return this->widgetContainer;
}

void VKWidget::resizeEvent(QResizeEvent *event)
{
    if (!this->isVisible())
    {
        return;
    }

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

void VKWidget::timerEvent(QTimerEvent *event)
{
    if (!this->isVisible())
    {
        this->killTimer(this->timerId);
        this->timerId = 0;
        return;
    }

    // only remove current timer
    // when setting the video size succeeds
    if (CoreSetVideoSize(this->width, this->height))
    {
        this->killTimer(this->timerId);
        this->timerId = 0;
    }
}
