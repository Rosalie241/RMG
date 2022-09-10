/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CustomButton.hpp"
#include "ControllerWidget.hpp"

using namespace UserInterface::Widget;

CustomButton::CustomButton(QWidget* parent) : QPushButton(parent)
{
	this->countDownTimer = new QTimer(this);	
    connect(this->countDownTimer, &QTimer::timeout, this, &CustomButton::on_countDownTimer_triggered);
}

CustomButton::~CustomButton()
{

}

void CustomButton::Initialize(QWidget* widget)
{
    this->controllerWidget = (ControllerWidget*)widget;

    connect(this, &QPushButton::released, 
    [=]
    {
        emit this->controllerWidget->on_CustomButton_released(this);
    });
}

void CustomButton::Clear()
{
    this->inputType = InputType::Invalid;
    this->inputData = 0;
    this->extraInputData = 0;
    this->savedInputData = 0;
    this->savedExtraInputData = 0;
    this->setText(" ");
}

void CustomButton::StartTimer()
{
    this->currentCountDownNum = 5;
    this->on_countDownTimer_triggered();
    this->countDownTimer->start(1000);
}

void CustomButton::StopTimer()
{
    this->countDownTimer->stop();
}

void CustomButton::SetInputData(enum InputType type, int value, int extraValue, QString name)
{
    this->inputType = type;
    this->inputData = value;
    this->extraInputData = extraValue;
    this->setText(name);
    this->StopTimer();
    emit this->controllerWidget->on_CustomButton_DataSet(this);
}

enum InputType CustomButton::GetInputType()
{
    return this->inputType;
}

int CustomButton::GetInputData()
{
    return this->inputData;
}

int CustomButton::GetExtraInputData()
{
    return this->extraInputData;
}

void CustomButton::SaveState()
{
    this->savedInputType = this->inputType;
    this->savedInputData = this->inputData;
    this->savedExtraInputData = this->extraInputData;
    this->savedText = this->text();
}

void CustomButton::RestoreState()
{
    this->inputType = this->savedInputType;
    this->inputData = this->savedInputData;
    this->extraInputData = this->savedExtraInputData;
    this->setText(this->savedText);
}

void CustomButton::on_countDownTimer_triggered()
{
    this->setText(QString::number(this->currentCountDownNum));

    if (this->currentCountDownNum == 0)
    {
        this->setText("");
        this->currentCountDownNum = 5;
        this->countDownTimer->stop();
        emit this->controllerWidget->on_CustomButton_TimerFinished(this);
    }

    this->currentCountDownNum--;
}
