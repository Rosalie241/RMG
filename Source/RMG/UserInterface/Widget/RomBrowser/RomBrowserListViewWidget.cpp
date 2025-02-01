/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomBrowserListViewWidget.hpp"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMimeData>

using namespace UserInterface::Widget;

RomBrowserListViewWidget::RomBrowserListViewWidget(QWidget* parent) : QTableView(parent)
{
#ifdef DRAG_DROP
    // configure drag & drop
    this->setDragDropMode(QAbstractItemView::DragDropMode::DropOnly);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(true);
#endif // DRAG_DROP
}

RomBrowserListViewWidget::~RomBrowserListViewWidget()
{
}

void RomBrowserListViewWidget::dragMoveEvent(QDragMoveEvent* event)
{
#ifdef DRAG_DROP
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData->hasUrls() || !mimeData->urls().first().isLocalFile())
    {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
#endif // DRAG_DROP
}

void RomBrowserListViewWidget::dragEnterEvent(QDragEnterEvent* event)
{
#ifdef DRAG_DROP
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData->hasUrls() || !mimeData->urls().first().isLocalFile())
    {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
#endif // DRAG_DROP
}

void RomBrowserListViewWidget::dropEvent(QDropEvent* event)
{
    emit this->FileDropped(event);
}

void RomBrowserListViewWidget::wheelEvent(QWheelEvent* event)
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

    QTableView::wheelEvent(event);
}

