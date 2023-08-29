/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MappingButton.hpp"
#include "ControllerWidget.hpp"

using namespace UserInterface::Widget;

MappingButton::MappingButton(QWidget* parent) : QPushButton(parent)
{
	this->countDownTimer = new QTimer(this);
    connect(this->countDownTimer, &QTimer::timeout, this, &MappingButton::on_countDownTimer_triggered);

    this->initialSize = this->size();
}

MappingButton::~MappingButton()
{

}

void MappingButton::Initialize(ControllerWidget* widget)
{
    this->controllerWidget = widget;

    connect(this, &QPushButton::released, [=, this]
    {
        emit this->controllerWidget->on_MappingButton_Released(this);
    });

    this->setText(" ");
}

void MappingButton::Clear()
{
    this->inputData.clear();
    this->savedInputData.clear();
    this->setText(" ");
}

void MappingButton::StartTimer()
{
    this->currentCountDownNum = 5;
    this->on_countDownTimer_triggered();
    this->countDownTimer->start(1000);
}

void MappingButton::StopTimer()
{
    this->countDownTimer->stop();
}

void MappingButton::SetInputData(enum InputType type, int value, int extraValue, QString name)
{
    // don't add stub
    if (name.simplified().isEmpty())
    {
        return;
    }

    this->inputData.clear();
    this->inputData.push_back(
        {
            type,
            value,
            extraValue,
            name
        }
    );

    this->reloadText();
    this->StopTimer();

    emit this->controllerWidget->on_MappingButton_DataSet(this);
}

void MappingButton::AddInputData(enum InputType type, int value, int extraValue, QString name)
{
    // don't add stub
    if (name.simplified().isEmpty())
    {
        return;
    }

    if (!this->HasInputData(type, value, extraValue))
    {
        this->inputData.push_back(
            {
                type,
                value,
                extraValue,
                name
            }
        );
    }

    this->reloadText();
    this->StopTimer();

    emit this->controllerWidget->on_MappingButton_DataSet(this);
}

void MappingButton::RemoveLastInputData(void)
{
    if (!this->inputData.empty())
    {
        this->inputData.pop_back();
    }

    this->reloadText();
}

void MappingButton::RemoveInputData(InputType type, int value, int extraValue)
{
    auto iter = this->getInputDataIter(type, value, extraValue);
    if (iter == this->inputData.end())
    {
        return;
    }

    this->inputData.erase(iter);
    this->reloadText();
}

bool MappingButton::HasInputData(enum InputType type, int value, int extraValue)
{
    return this->getInputDataIter(type, value, extraValue) != this->inputData.end();
}

std::vector<int> MappingButton::GetInputType()
{
    std::vector<int> inputTypeList;

    for (const auto& data : this->inputData)
    {
        inputTypeList.push_back((int)data.Type);
    }

    return inputTypeList;
}

std::vector<int> MappingButton::GetInputData()
{
    std::vector<int> inputDataList;

    for (const auto& data : this->inputData)
    {
        inputDataList.push_back(data.Data);
    }

    return inputDataList;
}

std::vector<int> MappingButton::GetExtraInputData()
{
    std::vector<int> extraInputDataList;

    for (const auto& data : this->inputData)
    {
        extraInputDataList.push_back(data.ExtraData);
    }

    return extraInputDataList;
}

std::vector<std::string> MappingButton::GetInputText()
{
    std::vector<std::string> inputTextList;

    for (const auto& data : this->inputData)
    {
        inputTextList.push_back(data.Text.toStdString());
    }

    return inputTextList;
}

void MappingButton::SaveState()
{
    this->savedInputData = this->inputData;
}

void MappingButton::RestoreState()
{
    this->inputData = this->savedInputData;
    this->reloadText();
}

void MappingButton::on_countDownTimer_triggered()
{
    this->setText(QString::number(this->currentCountDownNum));

    if (this->currentCountDownNum == 0)
    {
        this->setText("");
        this->currentCountDownNum = 5;
        this->countDownTimer->stop();
        emit this->controllerWidget->on_MappingButton_TimerFinished(this);
    }

    this->currentCountDownNum--;
}

void MappingButton::reloadText()
{
    QString text;
    QFontMetrics fontMetrics(this->font());

    int size = this->inputData.size();
    for (int i = 0; i < size; i++)
    {
        text += this->inputData.at(i).Text;

        // only add comma when
        // not at the last item
        if (i < (size - 1))
        {
            text += ", ";
        }
    }

    QString elidedText = fontMetrics.elidedText(text, Qt::ElideRight, this->initialSize.width());

    // needed for spacing
    if (elidedText.isEmpty())
    {
        elidedText = " ";
    }

    this->setToolTip(text);
    this->setText(elidedText);
}

std::vector<MappingButtonInputDataType>::iterator MappingButton::getInputDataIter(InputType type, int value, int extraValue)
{
    auto predicate = [type, value, extraValue](const auto& entry)
    {
        return entry.Type == type &&
                (value == -1      || entry.Data == value) &&
                (extraValue == -1 || entry.ExtraData == extraValue);
    };

    return std::find_if(this->inputData.begin(), this->inputData.end(), predicate);
}
