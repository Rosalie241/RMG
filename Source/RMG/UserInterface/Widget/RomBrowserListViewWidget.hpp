/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROMBROWSERLISTVIEWWIDGET_HPP
#define ROMBROWSERLISTVIEWWIDGET_HPP

#include <QTableView>
#include <QWheelEvent>

namespace UserInterface
{
namespace Widget
{
class RomBrowserListViewWidget : public QTableView
{
    Q_OBJECT
public:
    RomBrowserListViewWidget(QWidget* parent);
    ~RomBrowserListViewWidget();

protected:
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

signals:
    void ZoomIn(void);
    void ZoomOut(void);
};
} // namespace Widget
} // namespace UserInterface

#endif // ROMBROWSERLISTVIEWWIDGET_HPP
