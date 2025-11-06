/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CONTROLLERWIDGET_H
#define CONTROLLERWIDGET_H

#include "ControllerImageWidget.hpp"
#include "MappingButton.hpp"

#include "UserInterface/OptionsDialog.hpp"
#include "UserInterface/HotkeysDialog.hpp"
#include "UserInterface/EventFilter.hpp"
#include "Utilities/InputProfileDB.hpp"

using namespace UserInterface::Widget;

#include "ui_ControllerWidget.h"
#include "common.hpp"

#include <SDL3/SDL.h>

#include <RMG-Core/RomSettings.hpp>
#include <RMG-Core/RomHeader.hpp>
#include <RMG-Core/Settings.hpp>

namespace UserInterface
{
namespace Widget
{
class ControllerWidget : public QWidget, Ui::ControllerWidget
{
    Q_OBJECT

private:
    bool initialized = false;
    QString settingsSection;
    QString gameSection;

    OptionsDialogSettings optionsDialogSettings;

    MappingButton* currentButton = nullptr;
    bool addMappingToButton      = false;

    QList<QString> profiles;
    QList<QString> removedProfiles;
    QList<QString> addedProfiles;

    bool autoConfigButtonEnabled = false;

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

    struct inputDeviceData
    {
        InputDevice device;
        InputProfileDBEntry inputProfile;
    };

    QList<buttonWidgetMapping> buttonWidgetMappings;
    QList<axisWidgetMapping> joystickWidgetMappings;
    QList<buttonSettingMapping> buttonSettingMappings;
    QList<HotkeySettingMapping> hotkeySettingMappings;
    QList<MappingButton*> setupButtonWidgets;

    void initializeMappingButtons();
    void initializeProfileButtons();
    void initializeMiscButtons();

    bool isCurrentDeviceKeyboard();
    bool isCurrentDeviceNotFound();

    void disableAllChildren();
    void enableAllChildren();

    void removeDuplicates(MappingButton* button);

    QString getCurrentSettingsSection();

    QString getUserProfileSectionName(QString profile);

    bool isSectionUserProfile(QString section);
    bool isSectionGameProfile(QString section);

    void setPluggedIn(bool value);

    bool hasAnySettingChanged(QString section);

    void showErrorMessage(QString text, QString details = "");

    SDL_JoystickID currentJoystickId = -1;
    bool isCurrentJoystickGamepad = false;

    SDL_Joystick* currentJoystick = nullptr;
    SDL_Gamepad* currentGamepad = nullptr;

    int previousProfileComboBoxIndex = -1;

    bool onlyLoadGameProfile = false;
    CoreRomHeader gameRomHeader;
    CoreRomSettings gameRomSettings;

    bool allowKeyboardForAutomatic = false;

    HotkeysDialog* currentHotkeysDialog = nullptr;

public:
    ControllerWidget(QWidget* parent, EventFilter* eventFilter);
    ~ControllerWidget();

    void AddInputDevice(const InputDevice& device, const InputProfileDBEntry& inputProfile);
    void RemoveInputDevice(const InputDevice& device);
    void CheckInputDeviceSettings();
    void CheckInputDeviceSettings(QString section);

    void DrawControllerImage();
    void ClearControllerImage();

    void GetCurrentInputDevice(InputDevice& device, bool ignoreDeviceNotFound = false);
    bool IsPluggedIn();

    void SetOnlyLoadGameProfile(bool value, CoreRomHeader romHeader, CoreRomSettings romSettings);
    void SetAllowKeyboardForAutomatic(bool value);

    void SetSettingsSection(QString profile, QString section);
    void SetInitialized(bool value);

    void LoadSettings();
    void LoadSettings(QString section, bool loadUserProfile = false);
    void LoadUserProfileSettings();

    void SaveDefaultSettings();
    void SaveSettings();
    void SaveUserProfileSettings();
    void SaveSettings(QString section);

    void RevertSettings();

    void SetCurrentInputDevice(SDL_JoystickID joystickId, SDL_Joystick* joystick, 
                               SDL_Gamepad* gamepad, const InputProfileDBEntry& inputProfile);

    void AddUserProfile(QString name, QString section);
    void RemoveUserProfile(QString name, QString section);

private slots:
    void on_deadZoneSlider_valueChanged(int value);
    void on_analogStickSensitivitySlider_valueChanged(int value);
    
    void on_profileComboBox_currentIndexChanged(int value);

    void on_inputDeviceComboBox_currentIndexChanged(int value);
    void on_inputDeviceRefreshButton_clicked();

    void on_addProfileButton_clicked();
    void on_removeProfileButton_clicked();

    void on_autoConfigButton_clicked();
    void on_resetButton_clicked();
    void on_optionsButton_clicked();
    void on_hotkeysButton_clicked();

public slots:
    void on_MappingButton_Released(MappingButton* button);
    void on_MappingButton_TimerFinished(MappingButton* button);
    void on_MappingButton_DataSet(MappingButton* button);
    void on_MappingButton_Resized(MappingButton* button, QResizeEvent* event);
    
    void on_AddMappingButton_Released(MappingButton* button);
    void on_RemoveMappingButton_Released(MappingButton* button);

    void on_MainDialog_SdlEvent(SDL_Event* event);
    void on_MainDialog_SdlEventPollFinished();

signals:
    void CurrentInputDeviceChanged(ControllerWidget* widget, InputDevice device);
    void RefreshInputDevicesButtonClicked();

    void UserProfileAdded(QString name, QString section);
    void UserProfileRemoved(QString name, QString section);
};
}
}

#endif // CONTROLLERWIDGET_H