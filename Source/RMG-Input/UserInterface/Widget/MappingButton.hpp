/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAPPINGBUTTON_HPP
#define MAPPINGBUTTON_HPP

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

struct MappingButtonInputDataType
{
	InputType Type;
	int       Data;
	int       ExtraData;
	QString   Text;
};

class MappingButton : public QPushButton
{
private:
	QSize initialSize;
	QTimer* countDownTimer = nullptr;
	ControllerWidget* controllerWidget = nullptr;
	int currentCountDownNum = 0;

	std::vector<MappingButtonInputDataType> inputData;

	std::vector<MappingButtonInputDataType> savedInputData;

	void reloadText();

	std::vector<MappingButtonInputDataType>::iterator getInputDataIter(InputType type, int value, int extraValue);

public:
	MappingButton(QWidget* parent);
	~MappingButton();

	void Initialize(ControllerWidget* widget);
	void Clear();

	void StartTimer();
	void StopTimer();

	void SetInputData(InputType type, int value, int extraValue, QString name);
	void AddInputData(InputType type, int value, int extraValue, QString name);

	void RemoveLastInputData(void);
	void RemoveInputData(InputType type, int value, int extraValue);

	bool HasInputData(InputType type, int value = -1, int extraValue = -1);

	std::vector<int> GetInputType();
	std::vector<int> GetInputData();
	std::vector<int> GetExtraInputData();
	std::vector<std::string> GetInputText();

	void SaveState();
	void RestoreState();

private slots:
	void on_countDownTimer_triggered();
};
}
}

#endif // MAPPINGBUTTON_HPP
