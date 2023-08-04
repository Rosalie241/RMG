/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomBrowserEmptyWidget.hpp"

#include <QLabel>
#include <QHBoxLayout>

using namespace UserInterface::Widget;

RomBrowserEmptyWidget::RomBrowserEmptyWidget(QWidget* parent) : QWidget(parent)
{
    this->setupUi(this);
}

RomBrowserEmptyWidget::~RomBrowserEmptyWidget()
{
}

void RomBrowserEmptyWidget::on_selectRomDirectory_clicked(void)
{
    emit this->SelectRomDirectory();
}

void RomBrowserEmptyWidget::on_refreshButton_clicked(void)
{
    emit this->Refresh();
}

