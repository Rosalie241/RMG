/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RemoveMappingButton.hpp"
#include "ControllerWidget.hpp"
#include "MappingButton.hpp"

using namespace UserInterface::Widget;

RemoveMappingButton::RemoveMappingButton(QWidget* parent) : QPushButton(parent)
{
}

RemoveMappingButton::~RemoveMappingButton()
{
}

void RemoveMappingButton::Initialize(ControllerWidget* widget, MappingButton* button)
{
    this->controllerWidget = widget;
    this->mappingButton    = button;

    connect(this, &QPushButton::released, [this]
    {
        emit this->controllerWidget->on_RemoveMappingButton_Released(this->mappingButton);
    });
}
