/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CREATENETPLAYSESSIONEMPTYWIDGET_HPP
#define CREATENETPLAYSESSIONEMPTYWIDGET_HPP

#include <QWidget>

#include "ui_CreateNetplaySessionEmptyWidget.h"

namespace UserInterface
{
namespace Widget
{
class CreateNetplaySessionEmptyWidget : public QWidget, private Ui::CreateNetplaySessionEmptyWidget
{
    Q_OBJECT
public:
    CreateNetplaySessionEmptyWidget(QWidget* parent);
    ~CreateNetplaySessionEmptyWidget();
};
} // namespace Widget
} // namespace UserInterface

#endif // CREATENETPLAYSESSIONEMPTYWIDGET_HPP
