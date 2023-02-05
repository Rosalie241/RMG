/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CONTROLLERWIDGET_H
#define CONTROLLERWIDGET_H

#include "MappingButton.hpp"
#include "ControllerImageWidget.hpp"

#include "UserInterface/EventFilter.hpp"
#include "UserInterface/OptionsDialog.hpp"

using namespace UserInterface::Widget;

#include "ui_ControllerWidget.h"
#include "common.hpp"

#include <RMG-Core/Core.hpp>

#include <SDL.h>

namespace UserInterface
{
namespace Widget
{
class ControllerWidget : public QWidget, Ui::ControllerWidget
{
    Q_OBJECT

private:
    QString settingsSection;
    
    OptionsDialogSettings optionsDialogSettings;

    QList<QString> inputDeviceNameList;
    MappingButton* currentButton = nullptr;
    bool addMappingToButton      = false;
    
    struct buttonWidgetMapping
    {
        enum N64ControllerButton button;
        MappingButton* buttonWidget;
    };

    struct axisWidgetMapping
    {
        enum InputAxisDirection direction;
        MappingButton* buttonWidget;
    };

    struct buttonSettingMapping
    {
        MappingButton* button;
        SettingsID inputTypeSettingsId;
        SettingsID nameSettingsId;
        SettingsID dataSettingsId;
        SettingsID extraDataSettingsId;
    };

    QList<buttonWidgetMapping> buttonWidgetMappings;
    QList<axisWidgetMapping> joystickWidgetMappings;
    QList<buttonSettingMapping> buttonSettingMappings;
    QList<MappingButton*> setupButtonWidgets;

    void initializeButtons();

    bool isCurrentDeviceKeyboard();
    bool isCurrentDeviceNotFound();

    void disableAllChildren();
    void enableAllChildren();

    void removeDuplicates(MappingButton* button);

    QString getCurrentSettingsSection();

    SDL_JoystickID currentJoystickId     = -1;
    bool isCurrentJoystickGameController = false;

public:
    ControllerWidget(QWidget* parent, EventFilter* eventFilter);
    ~ControllerWidget();

    void AddInputDevice(QString deviceName, int deviceNum);
    void RemoveInputDevice(QString deviceName, int deviceNum);
    void CheckInputDeviceSettings();

    void DrawControllerImage();
    void ClearControllerImage();

    void GetCurrentInputDevice(QString& deviceName, int& deviceNum, bool ignoreDeviceNotFound = false);
    bool IsPluggedIn();

    void SetSettingsSection(QString section);

    void LoadSettings();
    void LoadSettings(QString section);
    void SaveDefaultSettings();
    void SaveSettings();

    void SetCurrentJoystickID(SDL_JoystickID joystickId);
    void SetIsCurrentJoystickGameController(bool isGameController);

private slots:
    void on_deadZoneSlider_valueChanged(int value);
    
    void on_profileComboBox_currentIndexChanged(int value);

    void on_inputDeviceComboBox_currentIndexChanged(int value);
    void on_inputDeviceRefreshButton_clicked();
    
    void on_controllerPluggedCheckBox_toggled(bool value);

    void on_removeProfileButton_clicked();

    void on_resetButton_clicked();
    void on_optionsButton_clicked();

public slots:
    void on_MappingButton_Released(MappingButton* button);
    void on_MappingButton_TimerFinished(MappingButton* button);
    void on_MappingButton_DataSet(MappingButton* button);
    
    void on_AddMappingButton_Released(MappingButton* button);
    void on_RemoveMappingButton_Released(MappingButton* button);

    void on_MainDialog_SdlEvent(SDL_Event* event);
    void on_MainDialog_SdlEventPollFinished();
signals:
    void CurrentInputDeviceChanged(ControllerWidget* widget, QString deviceName, int deviceNum);
    void RefreshInputDevicesButtonClicked();
};
}
}

#endif // CONTROLLERWIDGET_H