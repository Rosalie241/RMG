/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomBrowserGridViewWidget.hpp"

using namespace UserInterface::Widget;

RomBrowserGridViewWidget::RomBrowserGridViewWidget(QWidget* parent) : QListView(parent)
{
}

RomBrowserGridViewWidget::~RomBrowserGridViewWidget()
{
}

void RomBrowserGridViewWidget::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        int dy = event->angleDelta().y();
        if (dy > 0)
        { // zoom in
            emit this->ZoomIn();
        }
        else
        { // zoom out
            emit this->ZoomOut();
        }

        return;
    }

    QListView::wheelEvent(event);
}

