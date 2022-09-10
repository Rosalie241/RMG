/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CUSTOMBUTTON_HPP
#define CUSTOMBUTTON_HPP

#include <QPushButton>
#include <QTimer>

#include "common.hpp"

namespace UserInterface
{
namespace Widget
{

// we can't include ControllerWidget.hpp
// because that header includes this header,
// so forward-declare it to prevent issues
class ControllerWidget;

class CustomButton : public QPushButton
{
private:
	QTimer* countDownTimer = nullptr;
	ControllerWidget* controllerWidget = nullptr;
	int currentCountDownNum = 0;
	enum InputType inputType = InputType::Invalid;
	int inputData = 0;
	int extraInputData = 0;

	enum InputType savedInputType = InputType::Invalid;
	int savedInputData = 0;
	int savedExtraInputData = 0;
	QString savedText;
public:
	CustomButton(QWidget* parent);
	~CustomButton();
	void Initialize(QWidget* widget);
	void Clear();

	void StartTimer();
	void StopTimer();


	void SetInputData(enum InputType type, int value, int extraValue, QString name);
	enum InputType GetInputType();
	int GetInputData();
	int GetExtraInputData();

	void SaveState();
	void RestoreState();

private slots:
	void on_countDownTimer_triggered();
};
}
}


#endif // CUSTOMBUTTON_HPP
