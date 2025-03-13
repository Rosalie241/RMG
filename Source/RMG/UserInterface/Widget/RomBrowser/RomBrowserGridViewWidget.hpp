/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROMBROWSERGRIDVIEWWIDGET_HPP
#define ROMBROWSERGRIDVIEWWIDGET_HPP

#include <QWheelEvent>
#include <QListView>

namespace UserInterface
{
namespace Widget
{
class RomBrowserGridViewWidget : public QListView
{
    Q_OBJECT
public:
    RomBrowserGridViewWidget(QWidget* parent);
    ~RomBrowserGridViewWidget();

protected:
    void dragMoveEvent(QDragMoveEvent* event) Q_DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent* event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent* event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

signals:
    void ZoomIn(void);
    void ZoomOut(void);
    void FileDropped(QDropEvent* event);
};
} // namespace Widget
} // namespace UserInterface

#endif // ROMBROWSERGRIDVIEWWIDGET_HPP
