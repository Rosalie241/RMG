/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROMBROWSEREMPTYWIDGET_HPP
#define ROMBROWSEREMPTYWIDGET_HPP

#include <QWidget>

#include "ui_RomBrowserEmptyWidget.h"

namespace UserInterface
{
namespace Widget
{
class RomBrowserEmptyWidget : public QWidget, private Ui::RomBrowserEmptyWidget
{
    Q_OBJECT
public:
    RomBrowserEmptyWidget(QWidget* parent);
    ~RomBrowserEmptyWidget();

private slots:
    void on_selectRomDirectory_clicked(void);
    void on_refreshButton_clicked(void);

signals:
    void SelectRomDirectory(void);
    void Refresh(void);
};
} // namespace Widget
} // namespace UserInterface

#endif // ROMBROWSEREMPTYWIDGET_HPP
