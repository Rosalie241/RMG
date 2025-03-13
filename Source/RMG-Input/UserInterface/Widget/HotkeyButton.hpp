/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef HOTKEYBUTTON_HPP
#define HOTKEYBUTTON_HPP

#include <QPushButton>
#include <QTimer>

#include "common.hpp"

namespace UserInterface
{

// we can't include HotkeysDialog.hpp
// because that header includes this header,
// so forward-declare it to prevent issues
class HotkeysDialog;

namespace Widget
{

struct HotkeyButtonInputDataType
{
    InputType Type;
    int       Data;
    int       ExtraData;
    QString   Text;
};

class HotkeyButton : public QPushButton
{
private:
    QSize initialSize;
    QTimer* countDownTimer = nullptr;
    HotkeysDialog* hotkeysDialog = nullptr;
    int currentCountDownNum = 0;

    std::vector<HotkeyButtonInputDataType> inputData;

    std::vector<HotkeyButtonInputDataType> savedInputData;

    void reloadText();

    std::vector<HotkeyButtonInputDataType>::iterator getInputDataIter(InputType type, int value, int extraValue);

public:
    HotkeyButton(QWidget* parent);
    ~HotkeyButton();

    void Initialize(HotkeysDialog* dialog);
    void Clear();

    void StartTimer();
    void StopTimer();

    void AddInputData(InputType type, int value, int extraValue, QString name);

    bool HasInputData(InputType type, int value = -1, int extraValue = -1);

    std::vector<int> GetInputType();
    std::vector<int> GetInputData();
    std::vector<int> GetExtraInputData();
    std::vector<std::string> GetInputText();

    void SaveState();
    void RestoreState();
    void FinishState();

protected:
    void mousePressEvent(QMouseEvent* event);

private slots:
    void on_countDownTimer_triggered();
};

}
}

#endif // HOTKEYBUTTON_HPP