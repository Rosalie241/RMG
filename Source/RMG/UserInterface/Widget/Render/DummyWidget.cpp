/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "DummyWidget.hpp"

#include <QPalette>

using namespace UserInterface::Widget;

DummyWidget::DummyWidget(QWidget *parent) : QWidget(parent)
{
	QPalette blackPalette;
    blackPalette.setColor(QPalette::Window, Qt::black);
    this->setAutoFillBackground(true); 
    this->setPalette(blackPalette);
}

DummyWidget::~DummyWidget()
{
}
