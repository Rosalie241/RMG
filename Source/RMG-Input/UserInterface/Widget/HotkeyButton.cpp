/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "HotkeyButton.hpp"
#include "UserInterface/HotkeysDialog.hpp"

#include <QMouseEvent>

using namespace UserInterface::Widget;

HotkeyButton::HotkeyButton(QWidget* parent) : QPushButton(parent)
{
    this->countDownTimer = new QTimer(this);
    connect(this->countDownTimer, &QTimer::timeout, this, &HotkeyButton::on_countDownTimer_triggered);

    this->initialSize = this->size();
}

HotkeyButton::~HotkeyButton()
{

}

void HotkeyButton::Initialize(HotkeysDialog* dialog)
{
    this->hotkeysDialog = dialog;

    connect(this, &QPushButton::released, [=]
    {
        emit this->hotkeysDialog->on_HotkeyButton_Released(this);
    });

    this->setText(" ");
}

void HotkeyButton::Clear()
{
    this->inputData.clear();
    this->savedInputData.clear();
    this->setText("");
}

void HotkeyButton::StartTimer()
{
    // clear current state
    this->inputData.clear();

    this->currentCountDownNum = 5;
    this->on_countDownTimer_triggered();
    this->countDownTimer->start(1000);
}

void HotkeyButton::StopTimer()
{
    this->countDownTimer->stop();
}

void HotkeyButton::AddInputData(enum InputType type, int value, int extraValue, QString name)
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
}

bool HotkeyButton::HasInputData(enum InputType type, int value, int extraValue)
{
    return this->getInputDataIter(type, value, extraValue) != this->inputData.end();
}

std::vector<int> HotkeyButton::GetInputType()
{
    std::vector<int> inputTypeList;

    for (const auto& data : this->inputData)
    {
        inputTypeList.push_back((int)data.Type);
    }

    return inputTypeList;
}

std::vector<int> HotkeyButton::GetInputData()
{
    std::vector<int> inputDataList;

    for (const auto& data : this->inputData)
    {
        inputDataList.push_back(data.Data);
    }

    return inputDataList;
}

std::vector<int> HotkeyButton::GetExtraInputData()
{
    std::vector<int> extraInputDataList;

    for (const auto& data : this->inputData)
    {
        extraInputDataList.push_back(data.ExtraData);
    }

    return extraInputDataList;
}

std::vector<std::string> HotkeyButton::GetInputText()
{
    std::vector<std::string> inputTextList;

    for (const auto& data : this->inputData)
    {
        inputTextList.push_back(data.Text.toStdString());
    }

    return inputTextList;
}

void HotkeyButton::SaveState()
{
    this->savedInputData = this->inputData;
}

void HotkeyButton::RestoreState()
{
    this->inputData = this->savedInputData;
    this->reloadText();
}

void HotkeyButton::FinishState()
{
    this->reloadText();
    emit this->hotkeysDialog->on_HotkeyButton_StateFinished(this);
}

void HotkeyButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        this->Clear();
        return;
    }

    QPushButton::mousePressEvent(event);
}

void HotkeyButton::on_countDownTimer_triggered()
{
    this->setText(QString::number(this->currentCountDownNum));

    if (this->currentCountDownNum == 0)
    {
        this->setText("");
        this->currentCountDownNum = 5;
        this->countDownTimer->stop();
        emit this->hotkeysDialog->on_HotkeyButton_TimerFinished(this);
    }

    this->currentCountDownNum--;
}

void HotkeyButton::reloadText()
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
            text += "+";
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

std::vector<HotkeyButtonInputDataType>::iterator HotkeyButton::getInputDataIter(InputType type, int value, int extraValue)
{
    auto predicate = [type, value, extraValue](const auto& entry)
    {
        return entry.Type == type &&
                (value == -1      || entry.Data == value) &&
                (extraValue == -1 || entry.ExtraData == extraValue);
    };

    return std::find_if(this->inputData.begin(), this->inputData.end(), predicate);
}
