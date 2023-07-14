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
#include <algorithm>
#include <cmath>
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

void ControllerImageWidget::SetSensitivity(int value)
{
    if (this->sensitivityValue != value)
    {
        this->sensitivityValue = value;
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
        { N64ControllerButton::ZTrigger, ":Resource/Controller_Pressed_ZTrigger.svg" }
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
    const int width = rectF.width();
    const int height = rectF.height();
    // we'll move the analog stick by a percentage
    // of the total width/height from the image
    const double absoluteMaxOffset = ((double)(height * 0.12265f) / 2);
    // slope as in line gradient
    const double offsetSlope = absoluteMaxOffset / 100;
    // take sensitivity into account
    const double sensitivityFactor = this->sensitivityValue / 100.0;
    const double sensitivityAdjustedMaxOffset = absoluteMaxOffset * std::min(sensitivityFactor, 1.0);
    double offsetDeadzone = this->deadzoneValue * sensitivityFactor * offsetSlope;
    int offsetx = this->xAxisState * sensitivityFactor * offsetSlope;
    int offsety = this->yAxisState * sensitivityFactor * offsetSlope;
    const double offsetDist = std::hypot(offsetx, offsety);

    // take deadzone into account
    // deadzone grows with sensitivity such that the deadzone
    // is always a percentage of the real stick range
    if (offsetDist <= offsetDeadzone)
    {
        offsetx = 0;
        offsety = 0;
    }

    // take circle range into account
    if (offsetDist > sensitivityAdjustedMaxOffset)
    {
        offsetx = (offsetx / offsetDist) * sensitivityAdjustedMaxOffset;
        offsety = (offsety / offsetDist) * sensitivityAdjustedMaxOffset;
    }

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
