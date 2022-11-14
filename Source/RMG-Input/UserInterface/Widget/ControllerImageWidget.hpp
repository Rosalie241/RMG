/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CONTROLLERIMAGEWIDGET_HPP
#define CONTROLLERIMAGEWIDGET_HPP

#include <QWidget>

#include "common.hpp"

namespace UserInterface
{
namespace Widget
{
class ControllerImageWidget : public QWidget
{
    Q_OBJECT

private:
    // button state
    bool buttonState[(int)N64ControllerButton::Invalid] = {0};
    // x axis state, -100-100
    int xAxisState = 0;
    // y axis state, -100-100
    int yAxisState = 0;
    // deadzone value, 0-100
    int deadzoneValue = 0;
    // range value, 0-100
    int rangeValue = 100;

    bool needImageUpdate = false;
public:
    ControllerImageWidget(QWidget* parent);
    ~ControllerImageWidget();

    void SetButtonState(enum N64ControllerButton button, bool state);
    void SetXAxisState(int xAxis);
    void SetYAxisState(int yAxis);
    void SetDeadzone(int value);
    void SetRange(int value);

    void ClearControllerState();
    void UpdateImage();

    void paintEvent(QPaintEvent *event) override;
};
}
}

#endif // CONTROLLERIMAGEWIDGET_HPP