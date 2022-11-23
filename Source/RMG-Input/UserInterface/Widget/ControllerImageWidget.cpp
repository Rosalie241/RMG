/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ControllerImageWidget.hpp"

#include <QSvgRenderer>
#include <cmath>
#include <iostream>
#include <QPainter>
#include <QResizeEvent>

using namespace UserInterface::Widget;

ControllerImageWidget::ControllerImageWidget(QWidget* parent) : QWidget(parent)
{
}

ControllerImageWidget::~ControllerImageWidget()
{

}

void ControllerImageWidget::SetButtonState(enum N64ControllerButton button, bool state)
{
    if (this->buttonState[(int)button] != state)
    {
        this->buttonState[(int)button] = state;
        this->needImageUpdate = true;
    }
}

void ControllerImageWidget::SetXAxisState(int xAxis)
{
    if (this->xAxisState != xAxis)
    {
        this->xAxisState = xAxis;
        this->needImageUpdate = true;
    }
}

void ControllerImageWidget::SetYAxisState(int yAxis)
{
    if (this->yAxisState != yAxis)
    {
        this->yAxisState = yAxis;
        this->needImageUpdate = true;
    }
}

void ControllerImageWidget::SetDeadzone(int value)
{
    if (this->deadzoneValue != value)
    {
        this->deadzoneValue = value;
        this->needImageUpdate = true;
    }
}

void ControllerImageWidget::SetRange(int value)
{
    if (this->rangeValue != value)
    {
        this->rangeValue = value;
        this->needImageUpdate = true;
    }
}

void ControllerImageWidget::ClearControllerState()
{
    // reset button state
    for (int i = 0; i < (int)N64ControllerButton::Invalid; i++)
    {
        if (this->buttonState[i])
        {
            this->buttonState[i] = false;
            this->needImageUpdate = true;
        }
    }

    // reset x axis state
    if (this->xAxisState != 0)
    {
        this->xAxisState = 0;
        this->needImageUpdate = true;
    }

    // reset y axis state
    if (this->yAxisState != 0)
    {
        this->yAxisState = 0;
        this->needImageUpdate = true;
    }

    this->UpdateImage();
}

void ControllerImageWidget::UpdateImage()
{
    if (this->needImageUpdate)
    {
        this->update();
        this->needImageUpdate = false;
    }
}


void ControllerImageWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QSvgRenderer renderer;
 
    static const struct
    {
        enum N64ControllerButton button;
        QString imageUri;
    } buttons[] =
    {
        { N64ControllerButton::A, ":Resource/Controller_Pressed_A.svg" },
        { N64ControllerButton::B, ":Resource/Controller_Pressed_B.svg" },
        { N64ControllerButton::Start, ":Resource/Controller_Pressed_Start.svg" },
        { N64ControllerButton::DpadUp, ":Resource/Controller_Pressed_DpadUp.svg" },
        { N64ControllerButton::DpadDown, ":Resource/Controller_Pressed_DpadDown.svg" },
        { N64ControllerButton::DpadLeft, ":Resource/Controller_Pressed_DpadLeft.svg" },
        { N64ControllerButton::DpadRight, ":Resource/Controller_Pressed_DpadRight.svg" },
        { N64ControllerButton::CButtonUp, ":Resource/Controller_Pressed_CButtonUp.svg" },
        { N64ControllerButton::CButtonDown, ":Resource/Controller_Pressed_CButtonDown.svg" },
        { N64ControllerButton::CButtonLeft, ":Resource/Controller_Pressed_CButtonLeft.svg" },
        { N64ControllerButton::CButtonRight, ":Resource/Controller_Pressed_CButtonRight.svg" },
        { N64ControllerButton::LeftTrigger, ":Resource/Controller_Pressed_LeftTrigger.svg" },
        { N64ControllerButton::RightTrigger, ":Resource/Controller_Pressed_RightTrigger.svg" },
    };

    static const QString baseImageUri = ":Resource/Controller_NoAnalogStick.svg";
    static const QString analogStickImageUri = ":Resource/Controller_AnalogStick.svg";

    // render base image first
    renderer.load(baseImageUri);
    renderer.setAspectRatioMode(Qt::AspectRatioMode::KeepAspectRatio);
    renderer.render(&painter);

    // render button images on top
    // when the button is pressed
    for (auto& button : buttons)
    {
        if (this->buttonState[(int)button.button])
        {
            renderer.load(button.imageUri);
            renderer.setAspectRatioMode(Qt::AspectRatioMode::KeepAspectRatio);
            renderer.render(&painter);
        }
    }

    // render analog stick
    renderer.load(analogStickImageUri);
    renderer.setAspectRatioMode(Qt::AspectRatioMode::KeepAspectRatio);

    QRectF rectF = renderer.viewBoxF();
    double offsetx = 0, offsety = 0;
    const int width = rectF.width();
    const int height = rectF.height();
    // we'll move the analog stick by a percentage
    // of the total width/height from the image
    const double maxOffsety = ((double)(height * 0.12265f) / 2);
    const double maxOffsetx = maxOffsety;
    const double dist = sqrt(pow(this->xAxisState, 2) + pow(this->yAxisState, 2));

    int octagonX = 0, octagonY = 0;
    simulateOctagon(
        this->xAxisState / 100.0, // inputX
        this->yAxisState / 100.0, // inputY
        this->deadzoneValue / 100.0, // deadzoneFactor
        this->rangeValue / 100.0, // scalingFactor
        octagonX, // outputX
        octagonY // outputY
    );

    offsetx = octagonX * (maxOffsetx / N64_AXIS_PEAK);
    offsety = octagonY * (maxOffsety / N64_AXIS_PEAK);

    // adjust rect with offset
    rectF.adjust(
        // dx1
        offsetx,
        // dy1
        offsety,
        // dx2
        0,
        // dy2
        0
    );

    // make sure we keep the same 
    // width and height
    rectF.setWidth(width);
    rectF.setHeight(height);

    // set viewbox and render
    renderer.setViewBox(rectF);
    renderer.render(&painter);
}
