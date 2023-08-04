/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROMBROWSERGRIDVIEWWIDGET_HPP
#define ROMBROWSERGRIDVIEWWIDGET_HPP

#include <QListView>
#include <QWheelEvent>

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
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

signals:
    void ZoomIn(void);
    void ZoomOut(void);
};
} // namespace Widget
} // namespace UserInterface

#endif // ROMBROWSERGRIDVIEWWIDGET_HPP
