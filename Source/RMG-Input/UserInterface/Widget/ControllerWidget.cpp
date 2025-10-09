/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ControllerWidget.hpp"
#include "UserInterface/OptionsDialog.hpp"

#include "common.hpp"

#include <RMG-Core/RomSettings.hpp>
#include <RMG-Core/RomHeader.hpp>
#include <RMG-Core/Core.hpp>

#include <QInputDialog>
#include <QResizeEvent>
#include <QSvgRenderer>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>

#include <SDL3/SDL.h>

using namespace UserInterface::Widget;

ControllerWidget::ControllerWidget(QWidget* parent, EventFilter* eventFilter) : QWidget(parent)
{
    this->setupUi(this);

    // install eventFilter to all children and parents
    this->installEventFilter(eventFilter);
    for (auto& widget : this->children())
    {
        widget->installEventFilter(eventFilter);
    }
    QObject* parentObject = this->parent();
    while (parentObject != nullptr)
    {
        parentObject->installEventFilter(eventFilter);
        parentObject = parentObject->parent();
    }

    this->DrawControllerImage();

    this->buttonWidgetMappings.append(
    {
        { N64ControllerButton::A, this->aButton, },
        { N64ControllerButton::B, this->bButton, },
        { N64ControllerButton::Start, this->startButton },
        { N64ControllerButton::DpadUp, this->dpadUpButton },
        { N64ControllerButton::DpadDown, this->dpadDownButton },
        { N64ControllerButton::DpadLeft, this->dpadLeftButton },
        { N64ControllerButton::DpadRight, this->dpadRightButton },
        { N64ControllerButton::CButtonUp, this->cbuttonUpButton },
        { N64ControllerButton::CButtonDown, this->cbuttonDownButton },
        { N64ControllerButton::CButtonLeft, this->cbuttonLeftButton  },
        { N64ControllerButton::CButtonRight, this->cbuttonRightButton },
        { N64ControllerButton::LeftShoulder, this->leftShoulderButton },
        { N64ControllerButton::RightShoulder, this->rightShoulderButton },
        { N64ControllerButton::ZTrigger, this->zTriggerButton }
    });

    this->joystickWidgetMappings.append(
    {
        { InputAxisDirection::Up, this->analogStickUpButton },
        { InputAxisDirection::Down, this->analogStickDownButton },
        { InputAxisDirection::Left, this->analogStickLeftButton },
        { InputAxisDirection::Right, this->analogStickRightButton },
    });

    this->buttonSettingMappings.append(
    {
        { this->aButton, SettingsID::Input_A_InputType, SettingsID::Input_A_Name, SettingsID::Input_A_Data, SettingsID::Input_A_ExtraData },
        { this->bButton, SettingsID::Input_B_InputType, SettingsID::Input_B_Name, SettingsID::Input_B_Data, SettingsID::Input_B_ExtraData },
        { this->startButton, SettingsID::Input_Start_InputType, SettingsID::Input_Start_Name, SettingsID::Input_Start_Data, SettingsID::Input_Start_ExtraData },
        { this->dpadUpButton, SettingsID::Input_DpadUp_InputType, SettingsID::Input_DpadUp_Name, SettingsID::Input_DpadUp_Data, SettingsID::Input_DpadUp_ExtraData },
        { this->dpadDownButton, SettingsID::Input_DpadDown_InputType, SettingsID::Input_DpadDown_Name, SettingsID::Input_DpadDown_Data, SettingsID::Input_DpadDown_ExtraData },
        { this->dpadLeftButton, SettingsID::Input_DpadLeft_InputType, SettingsID::Input_DpadLeft_Name, SettingsID::Input_DpadLeft_Data, SettingsID::Input_DpadLeft_ExtraData },
        { this->dpadRightButton, SettingsID::Input_DpadRight_InputType, SettingsID::Input_DpadRight_Name, SettingsID::Input_DpadRight_Data, SettingsID::Input_DpadRight_ExtraData },
        { this->cbuttonUpButton, SettingsID::Input_CButtonUp_InputType, SettingsID::Input_CButtonUp_Name, SettingsID::Input_CButtonUp_Data, SettingsID::Input_CButtonUp_ExtraData },
        { this->cbuttonDownButton, SettingsID::Input_CButtonDown_InputType, SettingsID::Input_CButtonDown_Name, SettingsID::Input_CButtonDown_Data, SettingsID::Input_CButtonDown_ExtraData },
        { this->cbuttonLeftButton, SettingsID::Input_CButtonLeft_InputType, SettingsID::Input_CButtonLeft_Name, SettingsID::Input_CButtonLeft_Data, SettingsID::Input_CButtonLeft_ExtraData },
        { this->cbuttonRightButton, SettingsID::Input_CButtonRight_InputType, SettingsID::Input_CButtonRight_Name, SettingsID::Input_CButtonRight_Data, SettingsID::Input_CButtonRight_ExtraData },
        { this->leftShoulderButton, SettingsID::Input_LeftShoulder_InputType, SettingsID::Input_LeftShoulder_Name, SettingsID::Input_LeftShoulder_Data, SettingsID::Input_LeftShoulder_ExtraData },
        { this->rightShoulderButton, SettingsID::Input_RightShoulder_InputType, SettingsID::Input_RightShoulder_Name, SettingsID::Input_RightShoulder_Data, SettingsID::Input_RightShoulder_ExtraData },
        { this->zTriggerButton, SettingsID::Input_ZTrigger_InputType, SettingsID::Input_ZTrigger_Name, SettingsID::Input_ZTrigger_Data, SettingsID::Input_ZTrigger_ExtraData },
        { this->analogStickUpButton, SettingsID::Input_AnalogStickUp_InputType, SettingsID::Input_AnalogStickUp_Name, SettingsID::Input_AnalogStickUp_Data, SettingsID::Input_AnalogStickUp_ExtraData },
        { this->analogStickDownButton, SettingsID::Input_AnalogStickDown_InputType, SettingsID::Input_AnalogStickDown_Name, SettingsID::Input_AnalogStickDown_Data, SettingsID::Input_AnalogStickDown_ExtraData },
        { this->analogStickLeftButton, SettingsID::Input_AnalogStickLeft_InputType, SettingsID::Input_AnalogStickLeft_Name, SettingsID::Input_AnalogStickLeft_Data, SettingsID::Input_AnalogStickLeft_ExtraData },
        { this->analogStickRightButton, SettingsID::Input_AnalogStickRight_InputType, SettingsID::Input_AnalogStickRight_Name, SettingsID::Input_AnalogStickRight_Data, SettingsID::Input_AnalogStickRight_ExtraData },
    });

    this->hotkeySettingMappings.append(
    {
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_Shutdown_InputType, SettingsID::Input_Hotkey_Shutdown_Name, SettingsID::Input_Hotkey_Shutdown_Data, SettingsID::Input_Hotkey_Shutdown_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_Exit_InputType, SettingsID::Input_Hotkey_Exit_Name, SettingsID::Input_Hotkey_Exit_Data, SettingsID::Input_Hotkey_Exit_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SoftReset_InputType, SettingsID::Input_Hotkey_SoftReset_Name, SettingsID::Input_Hotkey_SoftReset_Data, SettingsID::Input_Hotkey_SoftReset_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_HardReset_InputType, SettingsID::Input_Hotkey_HardReset_Name, SettingsID::Input_Hotkey_HardReset_Data, SettingsID::Input_Hotkey_HardReset_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_Resume_InputType, SettingsID::Input_Hotkey_Resume_Name, SettingsID::Input_Hotkey_Resume_Data, SettingsID::Input_Hotkey_Resume_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_Screenshot_InputType, SettingsID::Input_Hotkey_Screenshot_Name, SettingsID::Input_Hotkey_Screenshot_Data, SettingsID::Input_Hotkey_Screenshot_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_LimitFPS_InputType, SettingsID::Input_Hotkey_LimitFPS_Name, SettingsID::Input_Hotkey_LimitFPS_Data, SettingsID::Input_Hotkey_LimitFPS_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor25_InputType, SettingsID::Input_Hotkey_SpeedFactor25_Name, SettingsID::Input_Hotkey_SpeedFactor25_Data, SettingsID::Input_Hotkey_SpeedFactor25_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor50_InputType, SettingsID::Input_Hotkey_SpeedFactor50_Name, SettingsID::Input_Hotkey_SpeedFactor50_Data, SettingsID::Input_Hotkey_SpeedFactor50_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor75_InputType, SettingsID::Input_Hotkey_SpeedFactor75_Name, SettingsID::Input_Hotkey_SpeedFactor75_Data, SettingsID::Input_Hotkey_SpeedFactor75_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor100_InputType, SettingsID::Input_Hotkey_SpeedFactor100_Name, SettingsID::Input_Hotkey_SpeedFactor100_Data, SettingsID::Input_Hotkey_SpeedFactor100_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor125_InputType, SettingsID::Input_Hotkey_SpeedFactor125_Name, SettingsID::Input_Hotkey_SpeedFactor125_Data, SettingsID::Input_Hotkey_SpeedFactor125_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor150_InputType, SettingsID::Input_Hotkey_SpeedFactor150_Name, SettingsID::Input_Hotkey_SpeedFactor150_Data, SettingsID::Input_Hotkey_SpeedFactor150_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor175_InputType, SettingsID::Input_Hotkey_SpeedFactor175_Name, SettingsID::Input_Hotkey_SpeedFactor175_Data, SettingsID::Input_Hotkey_SpeedFactor175_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor200_InputType, SettingsID::Input_Hotkey_SpeedFactor200_Name, SettingsID::Input_Hotkey_SpeedFactor200_Data, SettingsID::Input_Hotkey_SpeedFactor200_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor225_InputType, SettingsID::Input_Hotkey_SpeedFactor225_Name, SettingsID::Input_Hotkey_SpeedFactor225_Data, SettingsID::Input_Hotkey_SpeedFactor225_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor250_InputType, SettingsID::Input_Hotkey_SpeedFactor250_Name, SettingsID::Input_Hotkey_SpeedFactor250_Data, SettingsID::Input_Hotkey_SpeedFactor250_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor275_InputType, SettingsID::Input_Hotkey_SpeedFactor275_Name, SettingsID::Input_Hotkey_SpeedFactor275_Data, SettingsID::Input_Hotkey_SpeedFactor275_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SpeedFactor300_InputType, SettingsID::Input_Hotkey_SpeedFactor300_Name, SettingsID::Input_Hotkey_SpeedFactor300_Data, SettingsID::Input_Hotkey_SpeedFactor300_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveState_InputType, SettingsID::Input_Hotkey_SaveState_Name, SettingsID::Input_Hotkey_SaveState_Data, SettingsID::Input_Hotkey_SaveState_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_LoadState_InputType, SettingsID::Input_Hotkey_LoadState_Name, SettingsID::Input_Hotkey_LoadState_Data, SettingsID::Input_Hotkey_LoadState_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_GSButton_InputType, SettingsID::Input_Hotkey_GSButton_Name, SettingsID::Input_Hotkey_GSButton_Data, SettingsID::Input_Hotkey_GSButton_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot0_InputType, SettingsID::Input_Hotkey_SaveStateSlot0_Name, SettingsID::Input_Hotkey_SaveStateSlot0_Data, SettingsID::Input_Hotkey_SaveStateSlot0_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot1_InputType, SettingsID::Input_Hotkey_SaveStateSlot1_Name, SettingsID::Input_Hotkey_SaveStateSlot1_Data, SettingsID::Input_Hotkey_SaveStateSlot1_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot2_InputType, SettingsID::Input_Hotkey_SaveStateSlot2_Name, SettingsID::Input_Hotkey_SaveStateSlot2_Data, SettingsID::Input_Hotkey_SaveStateSlot2_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot3_InputType, SettingsID::Input_Hotkey_SaveStateSlot3_Name, SettingsID::Input_Hotkey_SaveStateSlot3_Data, SettingsID::Input_Hotkey_SaveStateSlot3_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot4_InputType, SettingsID::Input_Hotkey_SaveStateSlot4_Name, SettingsID::Input_Hotkey_SaveStateSlot4_Data, SettingsID::Input_Hotkey_SaveStateSlot4_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot5_InputType, SettingsID::Input_Hotkey_SaveStateSlot5_Name, SettingsID::Input_Hotkey_SaveStateSlot5_Data, SettingsID::Input_Hotkey_SaveStateSlot5_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot6_InputType, SettingsID::Input_Hotkey_SaveStateSlot6_Name, SettingsID::Input_Hotkey_SaveStateSlot6_Data, SettingsID::Input_Hotkey_SaveStateSlot6_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot7_InputType, SettingsID::Input_Hotkey_SaveStateSlot7_Name, SettingsID::Input_Hotkey_SaveStateSlot7_Data, SettingsID::Input_Hotkey_SaveStateSlot7_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot8_InputType, SettingsID::Input_Hotkey_SaveStateSlot8_Name, SettingsID::Input_Hotkey_SaveStateSlot8_Data, SettingsID::Input_Hotkey_SaveStateSlot8_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_SaveStateSlot9_InputType, SettingsID::Input_Hotkey_SaveStateSlot9_Name, SettingsID::Input_Hotkey_SaveStateSlot9_Data, SettingsID::Input_Hotkey_SaveStateSlot9_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_InputType, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_Name, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_Data, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_InputType, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_Name, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_Data, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_MemoryPak_InputType, SettingsID::Input_Hotkey_MemoryPak_Name, SettingsID::Input_Hotkey_MemoryPak_Data, SettingsID::Input_Hotkey_MemoryPak_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_RumblePak_InputType, SettingsID::Input_Hotkey_RumblePak_Name, SettingsID::Input_Hotkey_RumblePak_Data, SettingsID::Input_Hotkey_RumblePak_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_NoPak_InputType, SettingsID::Input_Hotkey_NoPak_Name, SettingsID::Input_Hotkey_NoPak_Data, SettingsID::Input_Hotkey_NoPak_ExtraData },
        { {}, {}, {}, {}, SettingsID::Input_Hotkey_Fullscreen_InputType, SettingsID::Input_Hotkey_Fullscreen_Name, SettingsID::Input_Hotkey_Fullscreen_Data, SettingsID::Input_Hotkey_Fullscreen_ExtraData },
    });

    MappingButton* buttonList[] =
    {
        // dpad
        this->dpadUpButton,
        this->dpadDownButton,
        this->dpadLeftButton,
        this->dpadRightButton,
        // analog stick
        this->analogStickUpButton,
        this->analogStickDownButton,
        this->analogStickLeftButton,
        this->analogStickRightButton,
        // cbuttons
        this->cbuttonUpButton,
        this->cbuttonDownButton,
        this->cbuttonLeftButton,
        this->cbuttonRightButton,
        // triggers
        this->leftShoulderButton,
        this->rightShoulderButton,
        this->zTriggerButton,
        // buttons
        this->aButton,
        this->bButton,
        this->startButton,
    };
    for (auto& button : buttonList)
    {
        this->setupButtonWidgets.append(button);
    }

    this->setPluggedIn(false);
    this->initializeMappingButtons();
    this->initializeProfileButtons();
    this->initializeMiscButtons();
}

ControllerWidget::~ControllerWidget()
{

}

void ControllerWidget::initializeMappingButtons()
{
    struct
    {
        MappingButton* mappingButton;
        AddMappingButton* addMappingButton;
        RemoveMappingButton* removeMappingButton;
    } mappingList[] =
    {
        // dpad
        { this->dpadUpButton, this->dpadUpAddButton, this->dpadUpRemoveButton },
        { this->dpadDownButton, this->dpadDownAddButton, this->dpadDownRemoveButton },
        { this->dpadLeftButton, this->dpadLeftAddButton, this->dpadLeftRemoveButton },
        { this->dpadRightButton, this->dpadRightAddButton, this->dpadRightRemoveButton },
        // analog stick
        { this->analogStickUpButton, this->analogStickUpAddButton, this->analogStickUpRemoveButton },
        { this->analogStickDownButton, this->analogStickDownAddButton, this->analogStickDownRemoveButton },
        { this->analogStickLeftButton, this->analogStickLeftAddButton, this->analogStickLeftRemoveButton },
        { this->analogStickRightButton, this->analogStickRightAddButton, this->analogStickRightRemoveButton },
        // cbuttons
        { this->cbuttonUpButton, this->cbuttonUpAddButton, this->cbuttonUpRemoveButton },
        { this->cbuttonDownButton, this->cbuttonDownAddButton, this->cbuttonDownRemoveButton },
        { this->cbuttonLeftButton, this->cbuttonLeftAddButton, this->cbuttonLeftRemoveButton },
        { this->cbuttonRightButton, this->cbuttonRightAddButton, this->cbuttonRightRemoveButton },
        // triggers
        { this->leftShoulderButton, this->leftShoulderAddButton, this->leftShoulderRemoveButton },
        { this->rightShoulderButton, this->rightShoulderAddButton, this->rightShoulderRemoveButton },
        { this->zTriggerButton, this->zTriggerAddButton, this->zTriggerRemoveButton },
        // buttons
        { this->aButton, this->aAddButton, this->aRemoveButton },
        { this->bButton, this->bAddButton, this->bRemoveButton },
        { this->startButton, this->startAddButton, this->startRemoveButton },
    };

    for (auto& mapping : mappingList)
    {
        // initialize buttons
        mapping.mappingButton->Initialize(this);
        mapping.addMappingButton->Initialize(this, mapping.mappingButton);
        mapping.removeMappingButton->Initialize(this, mapping.mappingButton);

        // clear text & set icon
        mapping.addMappingButton->setText("");
        mapping.removeMappingButton->setText("");
        mapping.addMappingButton->setIcon(QIcon::fromTheme("add-line"));
        mapping.removeMappingButton->setIcon(QIcon::fromTheme("delete-back-line"));
    }
}

void ControllerWidget::initializeProfileButtons()
{
    this->addProfileButton->setText("");
    this->addProfileButton->setIcon(QIcon::fromTheme("add-line"));
    this->removeProfileButton->setText("");
    this->removeProfileButton->setIcon(QIcon::fromTheme("delete-bin-line"));
}

void ControllerWidget::initializeMiscButtons()
{
    this->inputDeviceRefreshButton->setIcon(QIcon::fromTheme("refresh-line"));
    this->resetButton->setIcon(QIcon::fromTheme("restart-line"));
    this->optionsButton->setIcon(QIcon::fromTheme("settings-3-line"));
    this->hotkeysButton->setIcon(QIcon::fromTheme("gamepad-line"));
}

bool ControllerWidget::isCurrentDeviceKeyboard()
{
    const InputDevice device = this->inputDeviceComboBox->currentData().value<InputDevice>();

    return device.type == InputDeviceType::Keyboard ||
            (this->allowKeyboardForAutomatic && device.type == InputDeviceType::Automatic);
}

bool ControllerWidget::isCurrentDeviceNotFound()
{
    QString title = this->inputDeviceComboBox->currentText();
    return title.endsWith("(not found)");
}

void ControllerWidget::disableAllChildren()
{
    for (auto& object : this->children())
    {
        if (!object->inherits("QWidget"))
        {
            continue;
        }

        QWidget* widget = (QWidget*)object;
        if (widget->isEnabled())
        {
            widget->setEnabled(false);
        }
    }
}

void ControllerWidget::enableAllChildren()
{
    for (auto& object : this->children())
    {
        if (!object->inherits("QWidget"))
        {
            continue;
        }

        QWidget* widget = static_cast<QWidget*>(object);
        if (!widget->isEnabled())
        {
            widget->setEnabled(true);
        }
    }
}

void ControllerWidget::removeDuplicates(MappingButton* button)
{
    std::string section = this->getCurrentSettingsSection().toStdString();

    if (!this->optionsDialogSettings.RemoveDuplicateMappings)
    {
        return;
    }

    std::vector<int> inputType = button->GetInputType();
    std::vector<int> inputData = button->GetInputData();
    std::vector<int> extraInputData = button->GetExtraInputData();

    for (auto& buttonWidget : this->setupButtonWidgets)
    {
        // skip ourselfes
        if (buttonWidget == button)
        {
            continue;
        }

        for (size_t i = 0; i < inputType.size(); i++)
        {
            if (buttonWidget->HasInputData(static_cast<InputType>(inputType.at(i)), inputData.at(i), extraInputData.at(i)))
            {
                buttonWidget->RemoveInputData(static_cast<InputType>(inputType.at(i)), inputData.at(i), extraInputData.at(i));
            }
        }
    }
}

QString ControllerWidget::getCurrentSettingsSection()
{
    return this->profileComboBox->currentData().toString();
}

QString ControllerWidget::getUserProfileSectionName(QString profile)
{
    QString profileSection;

    profileSection = "Rosalie's Mupen GUI - Input Plugin User Profile \"";
    profileSection += profile;
    profileSection += "\"";

    return profileSection;
}

bool ControllerWidget::isSectionUserProfile(QString section)
{
    return section.contains("User Profile");
}

bool ControllerWidget::isSectionGameProfile(QString section)
{
    return !this->isSectionUserProfile(section) && section.contains("Game");
}

void ControllerWidget::setPluggedIn(bool value)
{
    QWidget* widgetList[] =
    {
        // dpad
        this->dpadGroupBox,
        this->dpadUpButton, this->dpadUpAddButton, this->dpadUpRemoveButton,
        this->dpadDownButton, this->dpadDownAddButton, this->dpadDownRemoveButton,
        this->dpadLeftButton, this->dpadLeftAddButton, this->dpadLeftRemoveButton,
        this->dpadRightButton, this->dpadRightAddButton, this->dpadRightRemoveButton,
        // analog stick
        this->analogStickGroupBox,
        this->analogStickUpButton, this->analogStickUpAddButton, this->analogStickUpRemoveButton,
        this->analogStickDownButton, this->analogStickDownAddButton, this->analogStickDownRemoveButton,
        this->analogStickLeftButton, this->analogStickLeftAddButton, this->analogStickLeftRemoveButton,
        this->analogStickRightButton, this->analogStickRightAddButton, this->analogStickRightRemoveButton,
        this->analogStickSensitivitySlider,
        // cbuttons
        this->cButtonsGroupBox,
        this->cbuttonUpButton, this->cbuttonUpAddButton, this->cbuttonUpRemoveButton,
        this->cbuttonDownButton, this->cbuttonDownAddButton, this->cbuttonDownRemoveButton,
        this->cbuttonLeftButton, this->cbuttonLeftAddButton, this->cbuttonLeftRemoveButton,
        this->cbuttonRightButton, this->cbuttonRightAddButton, this->cbuttonRightRemoveButton,
        // triggers
        this->analogStickGroupBox,
        this->leftShoulderButton, this->leftShoulderAddButton, this->leftShoulderRemoveButton,
        this->rightShoulderButton, this->rightShoulderAddButton, this->rightShoulderRemoveButton,
        this->zTriggerButton, this->zTriggerAddButton, this->zTriggerRemoveButton,
        // buttons
        this->buttonsGroupBox,
        this->aButton, this->aAddButton, this->aRemoveButton,
        this->bButton, this->bAddButton, this->bRemoveButton,
        this->startButton, this->startAddButton, this->startRemoveButton,
        // misc UI elements
        this->deadZoneGroupBox,
        this->deadZoneSlider,
        this->optionsButton,
        this->resetButton,
        this->hotkeysButton,
    };

    for (auto& widget : widgetList)
    {
        widget->setEnabled(value);
    }

    this->ClearControllerImage();
}

bool ControllerWidget::hasAnySettingChanged(QString sectionQString)
{
    std::string section = sectionQString.toStdString();

    // fallback to main section
    if (!CoreSettingsSectionExists(section))
    {
        section = this->settingsSection.toStdString();
    }

    // retrieve data from settings
    bool settingsIsPluggedIn        = CoreSettingsGetBoolValue(SettingsID::Input_PluggedIn, section);
    int settingsDeadZone            = CoreSettingsGetIntValue(SettingsID::Input_Deadzone, section);
    int settingsAnalogSensitivity   = CoreSettingsGetIntValue(SettingsID::Input_Sensitivity, section);
    int settingsPak                 = CoreSettingsGetIntValue(SettingsID::Input_Pak, section);
    std::string settingsGameboyRom  = CoreSettingsGetStringValue(SettingsID::Input_GameboyRom, section);
    std::string settingsGameboySave = CoreSettingsGetStringValue(SettingsID::Input_GameboySave, section);

    // retrieve current data
    bool currentIsPluggedIn        = this->inputDeviceComboBox->currentText() != "None";
    int currentDeadZone            = this->deadZoneSlider->value();
    int currentAnalogSensitivity   = this->analogStickSensitivitySlider->value();
    int currentPak                 = this->optionsDialogSettings.ControllerPak;
    std::string currentGameboyRom  = this->optionsDialogSettings.GameboyRom;
    std::string currentGameboySave = this->optionsDialogSettings.GameboySave;

    // compare settings with current data
    if (settingsIsPluggedIn       != currentIsPluggedIn ||
        settingsDeadZone          != currentDeadZone ||
        settingsAnalogSensitivity != currentAnalogSensitivity ||
        settingsPak               != currentPak ||
        settingsGameboyRom        != currentGameboyRom ||
        settingsGameboySave       != currentGameboySave)
    {
        return true;
    }

    // compare button mappings with settings
    for (auto& buttonSetting : this->buttonSettingMappings)
    {
        // retrieve data from settings
        std::vector<int> settingsTypes         = CoreSettingsGetIntListValue(buttonSetting.inputTypeSettingsId, section);
        std::vector<std::string> settingsNames = CoreSettingsGetStringListValue(buttonSetting.nameSettingsId, section);
        std::vector<int> settingsData          = CoreSettingsGetIntListValue(buttonSetting.dataSettingsId, section);
        std::vector<int> settingsExtraData     = CoreSettingsGetIntListValue(buttonSetting.extraDataSettingsId, section);

        // retrieve current data
        std::vector<int> currentTypes         = buttonSetting.button->GetInputType();
        std::vector<std::string> currentNames = buttonSetting.button->GetInputText();
        std::vector<int> currentData          = buttonSetting.button->GetInputData();
        std::vector<int> currentExtraData     = buttonSetting.button->GetExtraInputData();

        // compare data
        if (settingsTypes != currentTypes ||
            settingsNames != currentNames ||
            settingsData  != currentData  ||
            settingsExtraData != currentExtraData)
        {
            return true;
        }
    }

    // compare hotkey mappings with settings
    for (auto& hotkeySetting : this->hotkeySettingMappings)
    {
        // retrieve data from settings
        std::vector<int> settingsTypes         = CoreSettingsGetIntListValue(hotkeySetting.inputTypeSettingsId);
        std::vector<std::string> settingsNames = CoreSettingsGetStringListValue(hotkeySetting.nameSettingsId, section);
        std::vector<int> settingsData          = CoreSettingsGetIntListValue(hotkeySetting.dataSettingsId, section);
        std::vector<int> settingsExtraData     = CoreSettingsGetIntListValue(hotkeySetting.extraDataSettingsId, section);

        // retrieve current data
        std::vector<int> currentTypes         = hotkeySetting.inputTypes;
        std::vector<std::string> currentNames = hotkeySetting.inputText;
        std::vector<int> currentData          = hotkeySetting.inputData;
        std::vector<int> currentExtraData     = hotkeySetting.extraInputData;

        // compare data
        if (settingsTypes != currentTypes ||
            settingsNames != currentNames ||
            settingsData  != currentData  ||
            settingsExtraData != currentExtraData)
        {
            return true;
        }
    }

    // no differences found
    return false;
}

void ControllerWidget::showErrorMessage(QString text, QString details)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(text);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

void ControllerWidget::AddInputDevice(const InputDevice& device)
{
    QString deviceName = QString::fromStdString(device.name);
    QString name = deviceName;

    if (device.type == InputDeviceType::Joystick)
    {
        name += " (";
        name += QString::number(device.id);
        name += ")";
    }

    this->inputDeviceNameList.append(deviceName);
    this->inputDeviceComboBox->addItem(name, QVariant::fromValue<InputDevice>(device));
}

void ControllerWidget::RemoveInputDevice(const InputDevice& device)
{
    inputDeviceNameList.removeOne(QString::fromStdString(device.name));

    int index = this->inputDeviceComboBox->findData(QVariant::fromValue<InputDevice>(device));
    if (index >= 0)
    {
        this->inputDeviceComboBox->removeItem(index);
    }
}

void ControllerWidget::CheckInputDeviceSettings()
{
    this->CheckInputDeviceSettings(this->getCurrentSettingsSection());
}

void ControllerWidget::CheckInputDeviceSettings(QString sectionQString)
{
    std::string section = sectionQString.toStdString();

    // fallback to main section unless
    // it's a user profile
    if (!CoreSettingsSectionExists(section))
    {
        if (this->isSectionUserProfile(this->getCurrentSettingsSection()))
        {
            return;
        }

        if (!CoreSettingsSectionExists(this->settingsSection.toStdString()))
        {
            return;
        }

        section = this->settingsSection.toStdString();
    }

    bool isPluggedIn       = CoreSettingsGetBoolValue(SettingsID::Input_PluggedIn, section);
    std::string deviceName = CoreSettingsGetStringValue(SettingsID::Input_DeviceName, section);
    std::string devicePath = CoreSettingsGetStringValue(SettingsID::Input_DevicePath, section);
    std::string deviceSerial = CoreSettingsGetStringValue(SettingsID::Input_DeviceSerial, section);
    InputDeviceType deviceType;

    // keep compatibility with <v0.8.1
    if (CoreSettingsKeyExists(section, "DeviceType"))
    {
        deviceType = static_cast<InputDeviceType>(CoreSettingsGetIntValue(SettingsID::Input_DeviceType, section));
    }
    else
    {
        int deviceNum = CoreSettingsGetIntValue(SettingsID::Input_DeviceNum, section);
        switch (deviceNum)
        {
        case -4:
            deviceType = InputDeviceType::EmulateVRU;
            break;
        case -3:
            deviceType = InputDeviceType::None;
            break;
        case -2:
            deviceType = InputDeviceType::Automatic;
            break;
        case -1:
            deviceType = InputDeviceType::Keyboard;
            break;
        default:
            deviceType = InputDeviceType::Joystick;
            break;
        }
    }

    InputDevice device = { deviceType, deviceName, devicePath, deviceSerial };

    // do nothing when input device combobox
    // is empty
    if (this->inputDeviceComboBox->count() == 0)
    {
        return;
    }

    // clear (not found) devices first
    int notFoundIndex = this->inputDeviceComboBox->findText("(not found)", Qt::MatchFlag::MatchEndsWith);
    if (notFoundIndex != -1)
    {
        this->inputDeviceNameList.removeAt(notFoundIndex);
        this->inputDeviceComboBox->removeItem(notFoundIndex);
    }

    int deviceIndex = -1;
    int deviceSerialIndex = -1;

    for (int i = 0; i < this->inputDeviceComboBox->count(); i++)
    {
        InputDevice otherDevice = this->inputDeviceComboBox->itemData(i).value<InputDevice>();
        if (device.name == otherDevice.name &&
            device.serial == otherDevice.serial)
        {
            if (!device.serial.empty())
            {
                deviceSerialIndex = i;
            }

            if (device.path == otherDevice.path)
            {
                deviceIndex = i;
                break;
            }
        }
    }

    if (deviceIndex != -1)
    { // full match
        this->inputDeviceComboBox->setCurrentIndex(deviceIndex);

        // force-refresh automatic input device
        if (deviceType == InputDeviceType::Automatic)
        {
            this->on_inputDeviceComboBox_currentIndexChanged(deviceIndex);
        }
    }
    else if (deviceSerialIndex != -1)
    { // name and serial match
        this->inputDeviceComboBox->setCurrentIndex(deviceSerialIndex);
    }
    else
    { // no match
        QString title = QString::fromStdString(deviceName);
        title += " (not found)";
        this->inputDeviceNameList.append(QString::fromStdString(deviceName));
        this->inputDeviceComboBox->addItem(title, QVariant::fromValue<InputDevice>(device));
        this->inputDeviceComboBox->setCurrentIndex(this->inputDeviceNameList.count() - 1);
    }
}

void ControllerWidget::DrawControllerImage()
{
    this->controllerImageWidget->UpdateImage();
}

void ControllerWidget::ClearControllerImage()
{
    this->controllerImageWidget->ClearControllerState();
}

void ControllerWidget::GetCurrentInputDevice(InputDevice& device, bool ignoreDeviceNotFound)
{
    int currentIndex = this->inputDeviceComboBox->currentIndex();

    if (this->isCurrentDeviceNotFound() && !ignoreDeviceNotFound)
    {
        device = { };
    }
    else
    {
        device = this->inputDeviceComboBox->itemData(currentIndex).value<InputDevice>();
    }
}

void ControllerWidget::on_deadZoneSlider_valueChanged(int value)
{
    QString title;
    title = "Deadzone: ";
    title += QString::number(value);
    title += "%";

    this->deadZoneGroupBox->setTitle(title);
    this->controllerImageWidget->SetDeadzone(value);
}

void ControllerWidget::on_analogStickSensitivitySlider_valueChanged(int value)
{
    QString title = tr("Analog Stick Sensitivity: ");
    title += QString::number(value);
    title += "%";

    this->analogStickSensitivityGroupBox->setTitle(title);
    this->controllerImageWidget->SetSensitivity(value);
}

void ControllerWidget::on_profileComboBox_currentIndexChanged(int value)
{
    // save profile when switching profile
    if (this->initialized && this->previousProfileComboBoxIndex != -1)
    {
        // ensure the previous index doesn't go out of bounds
        if (this->previousProfileComboBoxIndex < this->profileComboBox->count())
        {
            QString section = this->profileComboBox->itemData(this->previousProfileComboBoxIndex).toString();
            this->SaveSettings(section);
        }
    }

    // update previous index
    this->previousProfileComboBoxIndex = value;

    // reload settings from section
    this->LoadSettings(this->getCurrentSettingsSection());
    // reload input device settings
    this->CheckInputDeviceSettings();
}

void ControllerWidget::on_inputDeviceComboBox_currentIndexChanged(int value)
{
    // do nothing when value is invalid
    if (value == -1)
    {
        return;
    }

    InputDevice device = this->inputDeviceComboBox->itemData(value).value<InputDevice>();

    this->ClearControllerImage();

    if (this->isCurrentDeviceNotFound())
    {
        device = { };
    }

    // set plugged in state
    this->setPluggedIn(device.type != InputDeviceType::None &&
                       device.type != InputDeviceType::EmulateVRU);

    emit this->CurrentInputDeviceChanged(this, device);
}

void ControllerWidget::on_inputDeviceRefreshButton_clicked()
{
    emit this->RefreshInputDevicesButtonClicked();
}

void ControllerWidget::on_addProfileButton_clicked()
{
    std::vector<std::string> profiles;
    std::vector<std::string>::iterator profilesIter;
    QString section;

    // retrieve profiles from settings
    profiles = CoreSettingsGetStringListValue(SettingsID::Input_Profiles);

    // ask user for a new profile name
    QString newProfile = QInputDialog::getText(this,
            "Create New Profile", "New profile name:", 
            QLineEdit::Normal, "", 
            nullptr,
            Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    if (newProfile.isEmpty())
    { // do nothing when user has canceled
        return;
    }

    // ensure profile doesn't contain ';' or '[]'
    if (newProfile.contains(';') ||
        newProfile.contains('[') ||
        newProfile.contains(']'))
    {
        this->showErrorMessage("Profile name cannot contain ';','[' or ']'!");
        return;
    }

    // ensure the name is unique
    profilesIter = std::find(profiles.begin(), profiles.end(), newProfile.toStdString());
    if (profilesIter != profiles.end())
    {
        this->showErrorMessage("Profile with the same name already exists!");
        return;
    }

    section = this->getUserProfileSectionName(newProfile);

    // add profile to UI
    this->profileComboBox->addItem(newProfile, section);
    this->profileComboBox->setCurrentText(newProfile);

    // add profile to settings
    profiles.push_back(newProfile.toStdString());
    CoreSettingsSetValue(SettingsID::Input_Profiles, profiles);

    // update added profiles
    if (!this->addedProfiles.contains(section))
    {
        this->addedProfiles.push_back(section);
    }

    // emit signal
    emit this->UserProfileAdded(newProfile, section);
}

void ControllerWidget::on_removeProfileButton_clicked()
{
    if (this->profileComboBox->currentData().toString() == this->settingsSection)
    {
        QMessageBox messageBox(this);
        messageBox.setIcon(QMessageBox::Icon::Warning);
        messageBox.setText("Are you sure you want to clear the main profile?");
        messageBox.addButton(QMessageBox::Yes);
        messageBox.addButton(QMessageBox::No);
        if (messageBox.exec() == QMessageBox::Yes)
        {
            this->SaveDefaultSettings();
            this->LoadSettings(this->getCurrentSettingsSection());
        }
        return;
    }

    QString currentSection = this->getCurrentSettingsSection();

    // add to removed profiles list (will be deleted later)
    if (!this->removedProfiles.contains(currentSection))
    {
        this->removedProfiles.push_back(currentSection);
    }
    this->addedProfiles.removeAll(currentSection);

    // if section is a user profile,
    // also remove it from the profiles list
    if (this->isSectionUserProfile(currentSection))
    {
        std::string userProfile = this->profileComboBox->currentText().toStdString();
        std::vector<std::string> userProfiles = CoreSettingsGetStringListValue(SettingsID::Input_Profiles);
        std::vector<std::string>::iterator userProfilesIter;

        userProfilesIter = std::find(userProfiles.begin(), userProfiles.end(), userProfile);
        if (userProfilesIter != userProfiles.end())
        {
            userProfiles.erase(userProfilesIter);
        }

        CoreSettingsSetValue(SettingsID::Input_Profiles, userProfiles);

        // remove item from UI
        this->profileComboBox->removeItem(this->profileComboBox->currentIndex());

        // emit signal
        emit this->UserProfileRemoved(this->profileComboBox->currentText(), currentSection);
    }

    // switch back to main profile when deleting
    // a per game profile
    if (this->isSectionGameProfile(currentSection))
    {
        // ensure we don't save the game specific profile
        // upon switching to the main profile
        this->previousProfileComboBoxIndex = -1;
        this->profileComboBox->setCurrentIndex(0);
    }

    // force a re-load when
    // we're only loading the game profile
    if (this->onlyLoadGameProfile)
    {
        this->LoadSettings(this->settingsSection);
    }
}

void ControllerWidget::on_resetButton_clicked()
{
    const QString section = this->getCurrentSettingsSection();
    const std::string sectionStr = section.toStdString();

    // revert settings in current section when it exists
    if (CoreSettingsSectionExists(sectionStr))
    {
        CoreSettingsRevertSection(sectionStr);
    }

    this->LoadSettings(section);
}

void ControllerWidget::on_optionsButton_clicked()
{
    InputDevice device = this->inputDeviceComboBox->currentData().value<InputDevice>();
    const bool isKeyboard = device.type == InputDeviceType::Keyboard;

    OptionsDialog dialog(this, this->optionsDialogSettings,
                         isKeyboard ? nullptr : this->currentJoystick, 
                         isKeyboard ? nullptr : this->currentController);
    int ret = dialog.exec();

    // when saved, store settings for later
    if (ret == QDialog::Accepted)
    {
        this->optionsDialogSettings = dialog.GetSettings();
    }
}

void ControllerWidget::on_hotkeysButton_clicked()
{
    HotkeysDialog dialog(this, this->hotkeySettingMappings,
                         this->isCurrentJoystickGameController,
                         this->currentJoystickId,
                         this->optionsDialogSettings.FilterEventsForButtons,
                         this->optionsDialogSettings.RemoveDuplicateMappings);

    // clear our UI state
    this->controllerImageWidget->ClearControllerState();

    this->currentHotkeysDialog = &dialog;

    int ret = dialog.exec();

    this->currentHotkeysDialog = nullptr;

    // when saved, store settings for later
    if (ret == QDialog::Accepted)
    {
        this->hotkeySettingMappings = dialog.GetSettingMappings();
    }
}

void ControllerWidget::on_MappingButton_Released(MappingButton* button)
{
    if (this->currentButton != nullptr)
    {
        this->currentButton->StopTimer();
        this->currentButton->RestoreState();
    }

    this->currentButton = button;
    button->SaveState();
    button->StartTimer();

    this->disableAllChildren();
}

void ControllerWidget::on_AddMappingButton_Released(MappingButton* button)
{
    this->addMappingToButton = true;
    button->click();
}

void ControllerWidget::on_RemoveMappingButton_Released(MappingButton* button)
{
    button->RemoveLastInputData();
}

void ControllerWidget::on_MappingButton_TimerFinished(MappingButton* button)
{
    if (this->currentButton == button)
    {
        this->currentButton      = nullptr;
        this->addMappingToButton = false;
    }

    button->RestoreState();

    this->enableAllChildren();
}

void ControllerWidget::on_MappingButton_DataSet(MappingButton* button)
{
    this->enableAllChildren();
    this->removeDuplicates(button);
    this->currentButton      = nullptr;
    this->addMappingToButton = false;
}

void ControllerWidget::on_MappingButton_Resized(MappingButton* button, QResizeEvent* event)
{
    QWidget* buttonList[] =
    {
        // dpad
        this->dpadUpAddButton, this->dpadUpRemoveButton,
        this->dpadDownAddButton, this->dpadDownRemoveButton,
        this->dpadLeftAddButton, this->dpadLeftRemoveButton,
        this->dpadRightAddButton, this->dpadRightRemoveButton,
        // analog stick
        this->analogStickUpAddButton, this->analogStickUpRemoveButton,
        this->analogStickDownAddButton, this->analogStickDownRemoveButton,
        this->analogStickLeftAddButton, this->analogStickLeftRemoveButton,
        this->analogStickRightAddButton, this->analogStickRightRemoveButton,
        // cbuttons
        this->cbuttonUpAddButton, this->cbuttonUpRemoveButton,
        this->cbuttonDownAddButton, this->cbuttonDownRemoveButton,
        this->cbuttonLeftAddButton, this->cbuttonLeftRemoveButton,
        this->cbuttonRightAddButton, this->cbuttonRightRemoveButton,
        // triggers
        this->leftShoulderAddButton, this->leftShoulderRemoveButton,
        this->rightShoulderAddButton, this->rightShoulderRemoveButton,
        this->zTriggerAddButton, this->zTriggerRemoveButton,
        // buttons
        this->aAddButton, this->aRemoveButton,
        this->bAddButton, this->bRemoveButton,
        this->startAddButton, this->startRemoveButton,
        // profile
        this->profileComboBox,
        this->addProfileButton, this->removeProfileButton,
        // input device
        this->inputDeviceComboBox,
        this->inputDeviceRefreshButton
    };

    const int height = event->size().height();

    for (auto& button : buttonList)
    {
        button->setFixedHeight(height);
    }
}

void ControllerWidget::on_MainDialog_SdlEvent(SDL_Event* event)
{
    // pass to hotkeys dialog when it's active
    if (this->currentHotkeysDialog != nullptr)
    {
        this->currentHotkeysDialog->on_MainDialog_SdlEvent(event);
        return;
    }

    switch (event->type)
    {
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        case SDL_EVENT_JOYSTICK_BUTTON_UP:
        { // gamepad & joystick button
            SDL_JoystickID joystickId = -1;
            InputType inputType = InputType::Invalid;
            int sdlButton = 0;
            bool sdlButtonPressed = false;
            QString sdlButtonName;

            if ((event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) ||
                (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP))
            { // gamepad button
                if (!this->isCurrentJoystickGameController &&
                    this->optionsDialogSettings.FilterEventsForButtons)
                {
                    return;
                }

                joystickId = event->gbutton.which;
                inputType = InputType::GamepadButton;
                sdlButton = event->gbutton.button;
                sdlButtonPressed = (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
                sdlButtonName = SDL_GetGamepadStringForButton((SDL_GamepadButton)sdlButton);
            }
            else if ((event->type == SDL_EVENT_JOYSTICK_BUTTON_DOWN) ||
                     (event->type == SDL_EVENT_JOYSTICK_BUTTON_UP))
            { // joystick button
                if (this->isCurrentJoystickGameController &&
                    this->optionsDialogSettings.FilterEventsForButtons)
                {
                    return;
                }

                joystickId = event->jbutton.which;
                inputType = InputType::JoystickButton;
                sdlButton = event->jbutton.button;
                sdlButtonPressed = (event->type == SDL_EVENT_JOYSTICK_BUTTON_DOWN);
                sdlButtonName = "button " + QString::number(sdlButton);
            }

            // make sure we have the right joystick
            if (joystickId != this->currentJoystickId)
            {
                break;
            }

            // handle button widget
            if (this->currentButton != nullptr)
            {
                if (sdlButtonPressed)
                {
                    if (this->addMappingToButton)
                    {
                        this->currentButton->AddInputData(
                            inputType,
                            sdlButton,
                            0,
                            sdlButtonName
                        );
                    }
                    else
                    {
                        this->currentButton->SetInputData(
                            inputType,
                            sdlButton,
                            0,
                            sdlButtonName
                        );
                    }
                }
                break;
            }

            // update controller button state
            for (auto& button : this->buttonWidgetMappings)
            {
                if (button.buttonWidget->HasInputData(inputType, sdlButton))
                {
                    this->controllerImageWidget->SetButtonState(button.button, sdlButtonPressed);
                }
            }

            // update controller analog stick state
            for (auto& joystick : this->joystickWidgetMappings)
            {
                if (joystick.buttonWidget->HasInputData(inputType, sdlButton))
                {
                    switch (joystick.direction)
                    {
                        case InputAxisDirection::Up:
                        case InputAxisDirection::Down:
                        {
                            const int value = (
                                joystick.direction == InputAxisDirection::Up ?
                                    100 :
                                    -100
                            );
                            this->controllerImageWidget->SetYAxisState(sdlButtonPressed ? value : 0);
                        } break;

                        case InputAxisDirection::Left:
                        case InputAxisDirection::Right:
                        {
                            const int value = (
                                joystick.direction == InputAxisDirection::Left ?
                                    100 :
                                    -100
                            );
                            this->controllerImageWidget->SetXAxisState(sdlButtonPressed ? value : 0);
                        } break;

                        default:
                            break;
                    }
                }
            }
        } break;

        case SDL_EVENT_JOYSTICK_HAT_MOTION:
        { // joystick hat
            SDL_JoystickID joystickId = event->jhat.which;
            InputType inputType = InputType::JoystickHat;
            int sdlHat          = event->jhat.hat;
            int sdlHatDirection = event->jhat.value;
            bool sdlHatCentered = (sdlHatDirection == SDL_HAT_CENTERED);
            QString sdlHatName  = "hat " + QString::number(sdlHat) + ":" + QString::number(sdlHatDirection);

            if (this->isCurrentJoystickGameController &&
                this->optionsDialogSettings.FilterEventsForButtons)
            {
                return;
            }

            // make sure we have the right joystick
            if (joystickId != this->currentJoystickId)
            {
                break;
            }

            // handle button widget
            if (this->currentButton != nullptr)
            {
                if (!sdlHatCentered)
                {
                    if (this->addMappingToButton)
                    {
                        this->currentButton->AddInputData(
                            inputType,
                            sdlHat,
                            sdlHatDirection,
                            sdlHatName
                        );
                    }
                    else
                    {
                        this->currentButton->SetInputData(
                            inputType,
                            sdlHat,
                            sdlHatDirection,
                            sdlHatName
                        );
                    }
                }
                break;
            }

            // when we receive a joystick hat event
            // that is centered, then we have to iterate
            // over every button widget which contains the
            // joystick hat with every possible direction,
            // and disable the button state, else when the 
            // joystick hat event isn't centered,
            // loop over every button widget that doesn't contain
            // the current direction and disable it.
            const uint8_t joystickHatDirections[] =
            {
                SDL_HAT_UP,    SDL_HAT_RIGHTUP,
                SDL_HAT_RIGHT, SDL_HAT_RIGHTDOWN,
                SDL_HAT_DOWN,  SDL_HAT_LEFTUP,
                SDL_HAT_LEFT,  SDL_HAT_LEFTDOWN,
            };
            for (auto& button : this->buttonWidgetMappings)
            {
                for (uint8_t direction : joystickHatDirections)
                {
                    if (button.buttonWidget->HasInputData(inputType, sdlHat, direction))
                    {
                        if (!sdlHatCentered && sdlHatDirection == direction)
                        {
                            this->controllerImageWidget->SetButtonState(button.button, true);
                        }
                        else
                        {
                            this->controllerImageWidget->SetButtonState(button.button, false);
                        }
                    }
                }
            }

            // we have to do update the analog stick state
            // the same way as above
            for (auto& joystick : this->joystickWidgetMappings)
            {
                for (uint8_t direction : joystickHatDirections)
                {
                    if (joystick.buttonWidget->HasInputData(inputType, sdlHat, direction))
                    {
                        switch (joystick.direction)
                        {
                            case InputAxisDirection::Up:
                            case InputAxisDirection::Down:
                            {
                                const int value = (
                                    joystick.direction == InputAxisDirection::Up ?
                                        100 :
                                        -100
                                );

                                if (!sdlHatCentered && sdlHatDirection == direction)
                                {
                                    this->controllerImageWidget->SetYAxisState(value);
                                }
                                else
                                {
                                    this->controllerImageWidget->SetYAxisState(0);
                                }
                            } break;

                            case InputAxisDirection::Left:
                            case InputAxisDirection::Right:
                            {
                                const int value = (
                                    joystick.direction == InputAxisDirection::Left ?
                                        100 :
                                        -100
                                );

                                if (!sdlHatCentered && sdlHatDirection == direction)
                                {
                                    this->controllerImageWidget->SetXAxisState(value);
                                }
                                else
                                {
                                    this->controllerImageWidget->SetXAxisState(0);
                                }
                            } break;

                            default:
                                break;
                        }
                    }
                }
            }
        } break;

        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        case SDL_EVENT_JOYSTICK_AXIS_MOTION:
        { // gamepad & joystick axis
            SDL_JoystickID joystickId = -1;
            InputType inputType = InputType::Invalid;
            int sdlAxis = 0;
            int sdlAxisValue = 0;
            QString sdlAxisName;

            if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
            { // gamepad axis
                if (!this->isCurrentJoystickGameController &&
                    this->optionsDialogSettings.FilterEventsForAxis)
                {
                    return;
                }

                joystickId = event->gaxis.which;
                inputType = InputType::GamepadAxis;
                sdlAxis = event->gaxis.axis;
                sdlAxisValue = event->gaxis.value;
                sdlAxisName = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)sdlAxis);
                sdlAxisName += sdlAxisValue > 0 ? "+" : "-";
            }
            else
            { // joystick axis
                if (this->isCurrentJoystickGameController &&
                    this->optionsDialogSettings.FilterEventsForAxis)
                {
                    return;
                }

                joystickId = event->jaxis.which;
                inputType = InputType::JoystickAxis;
                sdlAxis = event->jaxis.axis;
                sdlAxisValue = event->jaxis.value;
                sdlAxisName = "axis " + QString::number(sdlAxis);
                sdlAxisName += sdlAxisValue > 0 ? "+" : "-";
            }

            // make sure the user presses the axis
            // more than 50%, otherwise we might detect
            // an accidental axis movement (due to i.e deadzone)
            const bool sdlAxisButtonPressed = (abs(sdlAxisValue) >= (SDL_AXIS_PEAK / 2));
            int sdlAxisDirection = (sdlAxisValue > 0 ? 1 : 0);

            // make sure we have the right joystick
            if (joystickId != this->currentJoystickId)
            {
                break;
            }

            // handle button widget
            if (this->currentButton != nullptr)
            {
                if (sdlAxisButtonPressed)
                {
                    if (this->addMappingToButton)
                    {
                        this->currentButton->AddInputData(
                            inputType, 
                            sdlAxis,
                            sdlAxisDirection,
                            sdlAxisName
                        );
                    }
                    else
                    {
                        this->currentButton->SetInputData(
                            inputType, 
                            sdlAxis,
                            sdlAxisDirection,
                            sdlAxisName
                        );
                    }
                }
                break;
            }

            // update controller button state,
            // we have to loop over each possible direction,
            // then ensure the opposite direction is disabled
            // because an axis can only be at one direction
            // at a given time
            const int directions[] =
            {
                0, 1
            };
            for (auto& button : this->buttonWidgetMappings)
            {
                for (int direction : directions)
                {
                    if (button.buttonWidget->HasInputData(inputType, sdlAxis, direction))
                    {
                        this->controllerImageWidget->SetButtonState(button.button, (direction == sdlAxisDirection && sdlAxisButtonPressed));
                    }
                }
            }

            // update controller analog stick state
            for (auto& joystick : this->joystickWidgetMappings)
            {
                if (joystick.buttonWidget->HasInputData(inputType, sdlAxis, sdlAxisDirection))
                {
                    const int value = -(static_cast<double>(sdlAxisValue) / SDL_AXIS_PEAK * 100);

                    switch (joystick.direction)
                    {
                        case InputAxisDirection::Up:
                        case InputAxisDirection::Down:
                        {
                            this->controllerImageWidget->SetYAxisState(value);
                        } break;

                        case InputAxisDirection::Left:
                        case InputAxisDirection::Right:
                        {
                            this->controllerImageWidget->SetXAxisState(value);
                        } break;

                        default:
                            break;
                    }
                }
            }
        } break;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        { // keyboard button

            // make sure a keyboard is selected
            if (!this->isCurrentDeviceKeyboard())
            {
                break;
            }

            const SDL_Scancode sdlButton = (SDL_Scancode)event->key.scancode;
            const bool sdlButtonPressed = (event->type == SDL_EVENT_KEY_DOWN);

            // handle button widget
            if (this->currentButton != nullptr)
            {
                if (sdlButtonPressed)
                {
                    if (this->addMappingToButton)
                    {
                        this->currentButton->AddInputData(
                            InputType::Keyboard, 
                            sdlButton,
                            0,
                            SDL_GetScancodeName(sdlButton)
                        );
                    }
                    else
                    {
                        this->currentButton->SetInputData(
                            InputType::Keyboard, 
                            sdlButton,
                            0,
                            SDL_GetScancodeName(sdlButton)
                        );
                    }
                }
                break;
            }

            // update controller button state
            for (auto& button : this->buttonWidgetMappings)
            {
                if (button.buttonWidget->HasInputData(InputType::Keyboard, sdlButton))
                {
                    this->controllerImageWidget->SetButtonState(button.button, sdlButtonPressed);
                }
            }

            // update controller analog stick state
            for (auto& joystick : this->joystickWidgetMappings)
            {
                if (joystick.buttonWidget->HasInputData(InputType::Keyboard, sdlButton))
                {
                    switch (joystick.direction)
                    {
                        case InputAxisDirection::Up:
                        case InputAxisDirection::Down:
                        {
                            const int value = (
                                joystick.direction == InputAxisDirection::Up ?
                                    100 :
                                    -100
                            );
                            this->controllerImageWidget->SetYAxisState(sdlButtonPressed ? value : 0);
                        } break;

                        case InputAxisDirection::Left:
                        case InputAxisDirection::Right:
                        {
                            const int value = (
                                joystick.direction == InputAxisDirection::Left ?
                                    100 :
                                    -100
                            );
                            this->controllerImageWidget->SetXAxisState(sdlButtonPressed ? value : 0);
                        } break;

                        default:
                            break;
                    }
                }
            }

            break;
        }

        default:
            break;
    }
}

void ControllerWidget::on_MainDialog_SdlEventPollFinished()
{
    this->controllerImageWidget->UpdateImage();
}

bool ControllerWidget::IsPluggedIn()
{
    const InputDevice device = this->inputDeviceComboBox->currentData().value<InputDevice>();
    return device.type != InputDeviceType::None;
}

void ControllerWidget::SetOnlyLoadGameProfile(bool value, CoreRomHeader romHeader, CoreRomSettings romSettings)
{
    this->onlyLoadGameProfile = value;
    this->gameRomHeader   = romHeader;
    this->gameRomSettings = romSettings;

    // update UI element
    this->addProfileButton->setDisabled(value);
}

void ControllerWidget::SetAllowKeyboardForAutomatic(bool value)
{
    this->allowKeyboardForAutomatic = value;
}

void ControllerWidget::SetSettingsSection(QString profile, QString section)
{
    this->settingsSection = section;
    if (!this->onlyLoadGameProfile)
    {
        this->profileComboBox->addItem(profile, section);
    }
}

void ControllerWidget::SetInitialized(bool value)
{
    this->initialized = value;
}

void ControllerWidget::LoadSettings()
{
    if (this->settingsSection.isEmpty())
    {
        return;
    }

    QString section = this->settingsSection;
    std::vector<std::string> userProfiles;

    // if the main profile section doesn't exist,
    // save default settings
    if (!CoreSettingsSectionExists(section.toStdString()))
    {
        this->SaveDefaultSettings();
    }

    // try to retrieve the current rom's settings & header,
    // if that succeeds, we know we're ingame
    // and then we'll add a game specific profile to the combobox
    CoreRomSettings romSettings = this->gameRomSettings;
    CoreRomHeader romHeader     = this->gameRomHeader;
    if (this->onlyLoadGameProfile ||
        (CoreGetCurrentRomSettings(romSettings) &&
         CoreGetCurrentRomHeader(romHeader)))
    {
        this->gameSection = section + " Game " + QString::fromStdString(romSettings.MD5);

        // use internal rom name
        QString internalName = QString::fromStdString(romHeader.Name);

        // add game specific profile when 
        // it doesn't exist yet
        int index = this->profileComboBox->findData(this->gameSection);
        if (index == -1)
        {
            this->profileComboBox->addItem(internalName, this->gameSection);
        }

        // if a game specific section exists,
        // select it in the profile combobox
        // and use it to load the settings
        if (CoreSettingsSectionExists(this->gameSection.toStdString()) ||
            this->onlyLoadGameProfile)
        {
            // check if the key 'UseGameProfile' exists,
            // if it doesn't then use the profile,
            // else check if it's true and use it,
            // or if we're only loading the game profile
            if (this->onlyLoadGameProfile ||
                !CoreSettingsKeyExists(this->gameSection.toStdString(), "UseGameProfile") ||
                CoreSettingsGetBoolValue(SettingsID::Input_UseGameProfile, this->gameSection.toStdString()))
            {
                this->profileComboBox->setCurrentText(internalName);

                // only use game section if it exists
                if (CoreSettingsSectionExists(this->gameSection.toStdString()))
                {
                    section = this->gameSection;
                }
            }
        }
    }

    // try to add all the user's profiles when we're not only loading the
    // game profile
    if (!this->onlyLoadGameProfile)
    {
        // clear profiles list
        profiles.clear();

        userProfiles = CoreSettingsGetStringListValue(SettingsID::Input_Profiles);
        for (const std::string& profile : userProfiles)
        {
            QString profileSection = this->getUserProfileSectionName(QString::fromStdString(profile));

            // ensure the profile section exists,
            // if it does, add it when it doesn't
            // exist yet
            if (CoreSettingsSectionExists(profileSection.toStdString()))
            {
                // add to profiles list
                this->profiles.push_back(profileSection);

                int index = this->profileComboBox->findData(profileSection);
                if (index == -1)
                {
                    this->profileComboBox->addItem(QString::fromStdString(profile), profileSection);
                }
            }
        }
    }

    this->LoadSettings(section, true);
}

void ControllerWidget::LoadSettings(QString sectionQString, bool loadUserProfile)
{
    std::string section = sectionQString.toStdString();
    std::string useProfile;

    // if the removed profiles contain
    // the section, don't load it
    if (this->removedProfiles.contains(sectionQString) &&
        !this->addedProfiles.contains(sectionQString))
    {
        return;
    }

    // do nothing if the section doesn't exist
    if (!CoreSettingsSectionExists(section))
    {
        // fallback to main section for per game profiles
        if (this->isSectionGameProfile(sectionQString))
        {
            sectionQString = this->settingsSection;
            section        = this->settingsSection.toStdString();
        }
        else
        {
            return;
        }
    }

    // see if the profile contains the UseProfile setting,
    // if it does, see if that profile exists, then load
    // that instead
    useProfile = CoreSettingsGetStringValue(SettingsID::Input_UseProfile, section);
    if (!this->onlyLoadGameProfile && loadUserProfile && !useProfile.empty())
    {
        QString profileSection = this->getUserProfileSectionName(QString::fromStdString(useProfile));

        int index = this->profileComboBox->findData(profileSection);
        if (index != -1)
        {
            this->profileComboBox->setCurrentIndex(index);
            this->LoadSettings(profileSection);
            return;
        }
    }

    this->analogStickSensitivitySlider->setValue(CoreSettingsGetIntValue(SettingsID::Input_Sensitivity, section));
    this->deadZoneSlider->setValue(CoreSettingsGetIntValue(SettingsID::Input_Deadzone, section));
    this->optionsDialogSettings.RemoveDuplicateMappings = CoreSettingsGetBoolValue(SettingsID::Input_RemoveDuplicateMappings, section);
    this->optionsDialogSettings.ControllerPak = CoreSettingsGetIntValue(SettingsID::Input_Pak, section);
    this->optionsDialogSettings.GameboyRom = CoreSettingsGetStringValue(SettingsID::Input_GameboyRom, section);
    this->optionsDialogSettings.GameboySave = CoreSettingsGetStringValue(SettingsID::Input_GameboySave, section);
    this->optionsDialogSettings.FilterEventsForButtons = CoreSettingsGetBoolValue(SettingsID::Input_FilterEventsForButtons, section);
    this->optionsDialogSettings.FilterEventsForAxis = CoreSettingsGetBoolValue(SettingsID::Input_FilterEventsForAxis, section);

    for (auto& buttonSetting : this->buttonSettingMappings)
    {
        buttonSetting.button->Clear();

        std::vector<int> types         = CoreSettingsGetIntListValue(buttonSetting.inputTypeSettingsId, section);
        std::vector<std::string> names = CoreSettingsGetStringListValue(buttonSetting.nameSettingsId, section);
        std::vector<int> data          = CoreSettingsGetIntListValue(buttonSetting.dataSettingsId, section);
        std::vector<int> extraData     = CoreSettingsGetIntListValue(buttonSetting.extraDataSettingsId, section);

        int count = std::min(extraData.size(), std::min(types.size(), std::min(names.size(), data.size())));

        if (count == 0)
        { // attempt to load old profile type
            types.push_back(CoreSettingsGetIntValue(buttonSetting.inputTypeSettingsId, section));
            names.push_back(CoreSettingsGetStringValue(buttonSetting.nameSettingsId, section));
            data.push_back(CoreSettingsGetIntValue(buttonSetting.dataSettingsId, section));
            extraData.push_back(CoreSettingsGetIntValue(buttonSetting.extraDataSettingsId, section));
            count = 1;
        }

        // add all input data
        for (int i = 0; i < count; i++)
        {
            buttonSetting.button->AddInputData((InputType)types[i], data[i], extraData[i], QString::fromStdString(names.at(i)));
        }
    }

    for (auto& hotkeySetting : this->hotkeySettingMappings)
    {
        hotkeySetting.inputTypes     = CoreSettingsGetIntListValue(hotkeySetting.inputTypeSettingsId, section);
        hotkeySetting.inputText      = CoreSettingsGetStringListValue(hotkeySetting.nameSettingsId, section);
        hotkeySetting.inputData      = CoreSettingsGetIntListValue(hotkeySetting.dataSettingsId, section);
        hotkeySetting.extraInputData = CoreSettingsGetIntListValue(hotkeySetting.extraDataSettingsId, section);
    }

    // force refresh some UI elements
    this->CheckInputDeviceSettings(sectionQString);
    this->on_deadZoneSlider_valueChanged(this->deadZoneSlider->value());
    this->setPluggedIn(this->IsPluggedIn());
}

void ControllerWidget::LoadUserProfileSettings()
{
    QString section = this->getCurrentSettingsSection();

    if (!this->isSectionUserProfile(section))
    {
        return;
    }

    this->LoadSettings(section);
}

void ControllerWidget::SaveDefaultSettings()
{
    if (this->settingsSection.isEmpty())
    {
        return;
    }

    std::string section = this->settingsSection.toStdString();

    CoreSettingsSetValue(SettingsID::Input_PluggedIn, section, false);
    CoreSettingsSetValue(SettingsID::Input_DeviceName, section, std::string("None"));
    CoreSettingsSetValue(SettingsID::Input_DeviceType, section, static_cast<int>(InputDeviceType::None));
    CoreSettingsSetValue(SettingsID::Input_DevicePath, section, std::string(""));
    CoreSettingsSetValue(SettingsID::Input_DeviceSerial, section, std::string(""));
    CoreSettingsSetValue(SettingsID::Input_Deadzone, section, 9);
    CoreSettingsSetValue(SettingsID::Input_Sensitivity, section, 100);
    CoreSettingsSetValue(SettingsID::Input_Pak, section, 0);
    CoreSettingsSetValue(SettingsID::Input_RemoveDuplicateMappings, section, true);
    CoreSettingsSetValue(SettingsID::Input_FilterEventsForButtons, section, true);
    CoreSettingsSetValue(SettingsID::Input_FilterEventsForAxis, section, true);

    for (auto& buttonSetting : this->buttonSettingMappings)
    {
        CoreSettingsSetValue(buttonSetting.inputTypeSettingsId, section, std::vector<int>({ static_cast<int>(InputType::Invalid) }));
        CoreSettingsSetValue(buttonSetting.nameSettingsId, section, std::vector<std::string>({ std::string() }));
        CoreSettingsSetValue(buttonSetting.dataSettingsId, section, std::vector<int>({ 0 }));
        CoreSettingsSetValue(buttonSetting.extraDataSettingsId, section, std::vector<int>({ 0 }));
    }
}

void ControllerWidget::SaveSettings()
{
    if (this->settingsSection.isEmpty())
    {
        return;
    }

    // when we're only loading the game profile,
    // and the settings match the settings of the main
    // profile, see if the user has removed the game profile, 
    // if so, delete the section and return
    if (this->onlyLoadGameProfile &&
        !this->hasAnySettingChanged(this->settingsSection) &&
        !this->removedProfiles.empty())
    {
        std::string section = this->gameSection.toStdString();
        if (CoreSettingsSectionExists(section))
        {
            CoreSettingsDeleteSection(section);
        }
        return;
    }

    // when we're only loading the game profile,
    // we should only save when anything has changed
    if (this->onlyLoadGameProfile &&
        !this->hasAnySettingChanged(this->gameSection))
    {
        return;
    }

    this->SaveSettings(this->getCurrentSettingsSection());

    // we handle deletion of the game section above
    if (this->onlyLoadGameProfile)
    {
        return;
    }

    // delete all removed profile sections
    for (QString section : this->removedProfiles)
    {
        if (!this->addedProfiles.contains(section))
        {
            CoreSettingsDeleteSection(section.toStdString());
        }
    }
}

void ControllerWidget::SaveUserProfileSettings()
{
    QString section = this->getCurrentSettingsSection();

    // only save when we're a user profile
    if (!this->isSectionUserProfile(section))
    {
        return;
    }

    this->SaveSettings(section);
}

void ControllerWidget::SaveSettings(QString section)
{
    InputDevice device;

    std::string mainSettingsSection = this->settingsSection.toStdString();
    std::string sectionStr          = section.toStdString();

    // when the section is a user profile,
    // make sure we inform the profile to use
    // that user profile instead
    if (this->isSectionUserProfile(section))
    {
        CoreSettingsSetValue(SettingsID::Input_UseProfile, mainSettingsSection, this->profileComboBox->currentText().toStdString());
    }
    else
    {
        CoreSettingsSetValue(SettingsID::Input_UseProfile, mainSettingsSection, std::string(""));
    }

    // when section is a game specific profile,
    // make sure we tell it whether or not to use it
    if (this->isSectionGameProfile(section))
    {
        CoreSettingsSetValue(SettingsID::Input_UseGameProfile, sectionStr, true);
    }
    else if (!this->gameSection.isEmpty() &&
        CoreSettingsSectionExists(this->gameSection.toStdString()))
    {
        CoreSettingsSetValue(SettingsID::Input_UseGameProfile, this->gameSection.toStdString(), false);
    }

    this->GetCurrentInputDevice(device, true);

    CoreSettingsSetValue(SettingsID::Input_PluggedIn, sectionStr, this->IsPluggedIn());
    CoreSettingsSetValue(SettingsID::Input_DeviceName, sectionStr, device.name);
    CoreSettingsSetValue(SettingsID::Input_DeviceType, sectionStr, static_cast<int>(device.type));
    CoreSettingsSetValue(SettingsID::Input_DevicePath, sectionStr, device.path);
    CoreSettingsSetValue(SettingsID::Input_DeviceSerial, sectionStr, device.serial);
    CoreSettingsSetValue(SettingsID::Input_Deadzone, sectionStr, this->deadZoneSlider->value());
    CoreSettingsSetValue(SettingsID::Input_Sensitivity, sectionStr, this->analogStickSensitivitySlider->value());
    CoreSettingsSetValue(SettingsID::Input_Pak, sectionStr, this->optionsDialogSettings.ControllerPak);
    CoreSettingsSetValue(SettingsID::Input_GameboyRom, sectionStr, this->optionsDialogSettings.GameboyRom);
    CoreSettingsSetValue(SettingsID::Input_GameboySave, sectionStr, this->optionsDialogSettings.GameboySave);
    CoreSettingsSetValue(SettingsID::Input_RemoveDuplicateMappings, sectionStr, this->optionsDialogSettings.RemoveDuplicateMappings);
    CoreSettingsSetValue(SettingsID::Input_FilterEventsForButtons, sectionStr, this->optionsDialogSettings.FilterEventsForButtons);
    CoreSettingsSetValue(SettingsID::Input_FilterEventsForAxis, sectionStr, this->optionsDialogSettings.FilterEventsForAxis);

    for (auto& buttonSetting : this->buttonSettingMappings)
    {
        CoreSettingsSetValue(buttonSetting.inputTypeSettingsId, sectionStr, buttonSetting.button->GetInputType());
        CoreSettingsSetValue(buttonSetting.nameSettingsId, sectionStr, buttonSetting.button->GetInputText());
        CoreSettingsSetValue(buttonSetting.dataSettingsId, sectionStr, buttonSetting.button->GetInputData());
        CoreSettingsSetValue(buttonSetting.extraDataSettingsId, sectionStr, buttonSetting.button->GetExtraInputData());
    }

    for (auto& hotkeySetting : this->hotkeySettingMappings)
    {
        CoreSettingsSetValue(hotkeySetting.inputTypeSettingsId, sectionStr, hotkeySetting.inputTypes);
        CoreSettingsSetValue(hotkeySetting.nameSettingsId, sectionStr, hotkeySetting.inputText);
        CoreSettingsSetValue(hotkeySetting.dataSettingsId, sectionStr, hotkeySetting.inputData);
        CoreSettingsSetValue(hotkeySetting.extraDataSettingsId, sectionStr, hotkeySetting.extraInputData);
    }
}

void ControllerWidget::RevertSettings()
{
    QList<QString> sections;

    // add all sections from profiles
    sections.append(this->profiles);

    // add section with profiles list
    sections.append("Rosalie's Mupen GUI - Input Plugin");

    // revert each section
    for (QString section : sections)
    {
        CoreSettingsRevertSection(section.toStdString());
    }

    // don't compare profiles,
    // when we're only loading 
    // the game profile
    if (this->onlyLoadGameProfile)
    {
        return;
    }

    // delete all sections for added & removed profiles,
    // which aren't in the base profile section list
    sections.clear();
    sections.append(this->addedProfiles);
    sections.append(this->removedProfiles);
    for (QString section : sections)
    {
        if (!this->profiles.contains(section) &&
            CoreSettingsSectionExists(section.toStdString()))
        {
            CoreSettingsDeleteSection(section.toStdString());
        }
    }
}

void ControllerWidget::SetCurrentJoystickID(SDL_JoystickID joystickId)
{
    this->currentJoystickId = joystickId;
}

void ControllerWidget::SetIsCurrentJoystickGameController(bool isGameController)
{
    this->isCurrentJoystickGameController = isGameController;
}

void ControllerWidget::SetCurrentJoystick(SDL_Joystick* joystick, SDL_Gamepad* controller)
{
    this->currentJoystick   = joystick;
    this->currentController = controller;
}

void ControllerWidget::AddUserProfile(QString name, QString section)
{
    int index = this->profileComboBox->findData(section);
    if (index == -1)
    {
        this->profileComboBox->addItem(name, section);
    }
}

void ControllerWidget::RemoveUserProfile(QString name, QString section)
{
    int index = this->profileComboBox->findData(section);
    if (index != -1)
    {
        this->profileComboBox->removeItem(index);
    }
}

