/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef REMOVEMAPPINGBUTTON_HPP
#define REMOVEMAPPINGBUTTON_HPP

#include <QPushButton>

namespace UserInterface
{
namespace Widget
{

// we can't include ControllerWidget.hpp
// because that header includes this header,
// so forward-declare it to prevent issues
class ControllerWidget;
class MappingButton;

class RemoveMappingButton : public QPushButton
{
private:
    ControllerWidget* controllerWidget = nullptr;
    MappingButton* mappingButton       = nullptr;

public:
    RemoveMappingButton(QWidget* parent);
    ~RemoveMappingButton();

    void Initialize(ControllerWidget* widget, MappingButton* button);
};
}
}

#endif // REMOVEMAPPINGBUTTON_HPP
