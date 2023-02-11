/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ControllerWidget.hpp"
#include "UserInterface/OptionsDialog.hpp"

#include "common.hpp"

#include <RMG-Core/Core.hpp>

#include <QPixmap>
#include <QPainter>
#include <QResizeEvent>
#include <QSvgRenderer>
#include <QMessageBox>
#include <QInputDialog>
#include <SDL.h>
#include <iostream>

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
        { N64ControllerButton::LeftTrigger, this->leftTriggerButton },
        { N64ControllerButton::RightTrigger, this->rightTriggerButton },
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
        { this->leftTriggerButton, SettingsID::Input_LeftTrigger_InputType, SettingsID::Input_LeftTrigger_Name, SettingsID::Input_LeftTrigger_Data, SettingsID::Input_LeftTrigger_ExtraData },
        { this->rightTriggerButton, SettingsID::Input_RightTrigger_InputType, SettingsID::Input_RightTrigger_Name, SettingsID::Input_RightTrigger_Data, SettingsID::Input_RightTrigger_ExtraData },
        { this->zTriggerButton, SettingsID::Input_ZTrigger_InputType, SettingsID::Input_ZTrigger_Name, SettingsID::Input_ZTrigger_Data, SettingsID::Input_ZTrigger_ExtraData },
        { this->analogStickUpButton, SettingsID::Input_AnalogStickUp_InputType, SettingsID::Input_AnalogStickUp_Name, SettingsID::Input_AnalogStickUp_Data, SettingsID::Input_AnalogStickUp_ExtraData },
        { this->analogStickDownButton, SettingsID::Input_AnalogStickDown_InputType, SettingsID::Input_AnalogStickDown_Name, SettingsID::Input_AnalogStickDown_Data, SettingsID::Input_AnalogStickDown_ExtraData },
        { this->analogStickLeftButton, SettingsID::Input_AnalogStickLeft_InputType, SettingsID::Input_AnalogStickLeft_Name, SettingsID::Input_AnalogStickLeft_Data, SettingsID::Input_AnalogStickLeft_ExtraData },
        { this->analogStickRightButton, SettingsID::Input_AnalogStickRight_InputType, SettingsID::Input_AnalogStickRight_Name, SettingsID::Input_AnalogStickRight_Data, SettingsID::Input_AnalogStickRight_ExtraData },
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
        this->leftTriggerButton,
        this->rightTriggerButton,
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
        { this->leftTriggerButton, this->leftTriggerAddButton, this->leftTriggerRemoveButton },
        { this->rightTriggerButton, this->rightTriggerAddButton, this->rightTriggerRemoveButton },
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
}

bool ControllerWidget::isCurrentDeviceKeyboard()
{
    int deviceNum = this->inputDeviceComboBox->currentData().toInt();

    return deviceNum == (int)InputDeviceType::Automatic ||
            deviceNum == (int)InputDeviceType::Keyboard;
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

        QWidget* widget = (QWidget*)object;
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

    for (auto& buttonWidget : this->setupButtonWidgets)
    {
        // skip ourselfes
        if (buttonWidget == button)
        {
            continue;
        }

        std::vector<int> inputType;
        std::vector<int> inputData;
        std::vector<int> extraInputData;

        inputType = button->GetInputType();
        inputData = button->GetInputData();
        extraInputData = button->GetExtraInputData();

        for (int i = 0; i < inputType.size(); i++)
        {
            if (buttonWidget->HasInputData((InputType)inputType.at(i), inputData.at(i), extraInputData.at(i)))
            {
                buttonWidget->RemoveInputData((InputType)inputType.at(i), inputData.at(i), extraInputData.at(i));
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
        // cbuttons
        this->cButtonsGroupBox,
        this->cbuttonUpButton, this->cbuttonUpAddButton, this->cbuttonUpRemoveButton,
        this->cbuttonDownButton, this->cbuttonDownAddButton, this->cbuttonDownRemoveButton,
        this->cbuttonLeftButton, this->cbuttonLeftAddButton, this->cbuttonLeftRemoveButton,
        this->cbuttonRightButton, this->cbuttonRightAddButton, this->cbuttonRightRemoveButton,
        // triggers
        this->analogStickGroupBox,
        this->leftTriggerButton, this->leftTriggerAddButton, this->leftTriggerRemoveButton,
        this->rightTriggerButton, this->rightTriggerAddButton, this->rightTriggerRemoveButton,
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
        this->resetButton
    };

    for (auto& widget : widgetList)
    {
        widget->setEnabled(value);
    }

    this->ClearControllerImage();
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

void ControllerWidget::AddInputDevice(QString deviceName, int deviceNum)
{
    QString name = deviceName;

    if (deviceNum >= 0)
    {
        name += " (";
        name += QString::number(deviceNum);
        name += ")";
    }

    this->inputDeviceNameList.append(deviceName);
    this->inputDeviceComboBox->addItem(name, deviceNum);
}

void ControllerWidget::RemoveInputDevice(QString deviceName, int deviceNum)
{
    inputDeviceNameList.removeOne(deviceName);

    for (int i = 0; i < this->inputDeviceComboBox->count(); i++)
    {
        int tmpNum = this->inputDeviceComboBox->itemData(i).toInt();
        QString tmpName = this->inputDeviceComboBox->itemText(i);

        if (tmpName.contains(deviceName) && deviceNum == tmpNum)
        {
            this->inputDeviceComboBox->removeItem(i);
            break;
        }
    }
}

void ControllerWidget::CheckInputDeviceSettings()
{
    std::string section = this->getCurrentSettingsSection().toStdString();

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
    int deviceNum          = CoreSettingsGetIntValue(SettingsID::Input_DeviceNum, section);

    // do nothing when input device combobox
    // is empty
    if (this->inputDeviceComboBox->count() == 0)
    {
        return;
    }

    // account for old setting
    if (!isPluggedIn && deviceNum != (int)InputDeviceType::None)
    {
        deviceName = "None";
        deviceNum  = (int)InputDeviceType::None;
    }

    // clear (not found) devices first
    int notFoundIndex = this->inputDeviceComboBox->findText("(not found)", Qt::MatchFlag::MatchEndsWith);
    if (notFoundIndex != -1)
    {
        this->inputDeviceNameList.removeAt(notFoundIndex);
        this->inputDeviceComboBox->removeItem(notFoundIndex);
    }

    int deviceNameIndex = this->inputDeviceComboBox->findText(QString::fromStdString(deviceName), Qt::MatchFlag::MatchStartsWith);
    int deviceNumIndex  = this->inputDeviceComboBox->findData(deviceNum);

    if ((deviceNumIndex != -1) &&
        (this->inputDeviceComboBox->itemText(deviceNumIndex).startsWith(QString::fromStdString(deviceName))))
    { // full match
        this->inputDeviceComboBox->setCurrentIndex(deviceNumIndex);

        // force-refresh automatic input device
        if (deviceNum == (int)InputDeviceType::Automatic)
        {
            this->on_inputDeviceComboBox_currentIndexChanged(deviceNumIndex);
        }
    }
    else if (deviceNameIndex != -1)
    { // name only match
        this->inputDeviceComboBox->setCurrentIndex(deviceNameIndex);
    }
    else
    { // no match
        QString title = QString::fromStdString(deviceName);
        title += " (not found)";
        this->inputDeviceNameList.append(QString::fromStdString(deviceName));
        this->inputDeviceComboBox->addItem(title, deviceNum);
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

void ControllerWidget::GetCurrentInputDevice(QString& deviceName, int& deviceNum, bool ignoreDeviceNotFound)
{
    int currentIndex = this->inputDeviceComboBox->currentIndex();

    if (this->isCurrentDeviceNotFound() && !ignoreDeviceNotFound)
    {
        deviceName = "";
        deviceNum  = -1;
    }
    else
    {
        deviceName = this->inputDeviceNameList.at(currentIndex);
        deviceNum  = this->inputDeviceComboBox->itemData(currentIndex).toInt();
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

    QString deviceName = this->inputDeviceNameList.at(value);
    int deviceNum      = this->inputDeviceComboBox->itemData(value).toInt();

    this->ClearControllerImage();

    if (this->isCurrentDeviceNotFound())
    {
        deviceName = "";
        deviceNum  = -1;
    }

    // set plugged in state
    this->setPluggedIn(deviceNum != (int)InputDeviceType::None);

    emit this->CurrentInputDeviceChanged(this, deviceName, deviceNum);
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
    bool ret;

    if (this->profileComboBox->currentIndex() == 0)
    {
        QMessageBox messageBox(this);
        messageBox.setIcon(QMessageBox::Icon::Warning);
        messageBox.setText("Are you sure you want to reset the main profile?");
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
}

void ControllerWidget::on_resetButton_clicked()
{
    this->LoadSettings(this->getCurrentSettingsSection());
}

void ControllerWidget::on_optionsButton_clicked()
{
    OptionsDialog dialog(this, this->optionsDialogSettings);
    dialog.exec();

    // when saved, store settings for later
    if (dialog.HasSaved())
    {
        this->optionsDialogSettings = dialog.GetSettings();
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

void ControllerWidget::on_MainDialog_SdlEvent(SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        { // gamepad & joystick button
            SDL_JoystickID joystickId = -1;
            InputType inputType = InputType::Invalid;
            int sdlButton = 0;
            bool sdlButtonPressed = false;
            QString sdlButtonName;

            if ((event->type == SDL_CONTROLLERBUTTONDOWN) ||
                (event->type == SDL_CONTROLLERBUTTONUP))
            { // gamepad button
                if (!this->isCurrentJoystickGameController &&
                    this->optionsDialogSettings.FilterEventsForButtons)
                {
                    return;
                }

                joystickId = event->cbutton.which;
                inputType = InputType::GamepadButton;
                sdlButton = event->cbutton.button;
                sdlButtonPressed = (event->type == SDL_CONTROLLERBUTTONDOWN);
                sdlButtonName = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)sdlButton);
            }
            else
            { // joystick button
                if (this->isCurrentJoystickGameController &&
                    this->optionsDialogSettings.FilterEventsForButtons)
                {
                    return;
                }

                joystickId = event->jbutton.which;
                inputType = InputType::JoystickButton;
                sdlButton = event->jbutton.button;
                sdlButtonPressed = (event->type == SDL_JOYBUTTONDOWN);
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

        case SDL_CONTROLLERAXISMOTION:
        case SDL_JOYAXISMOTION:
        { // gamepad & joystick axis
            SDL_JoystickID joystickId = -1;
            InputType inputType = InputType::Invalid;
            int sdlAxis = 0;
            int sdlAxisValue = 0;
            QString sdlAxisName;

            if (event->type == SDL_CONTROLLERAXISMOTION)
            { // gamepad axis
                if (!this->isCurrentJoystickGameController &&
                    this->optionsDialogSettings.FilterEventsForAxis)
                {
                    return;
                }

                joystickId = event->caxis.which;
                inputType = InputType::GamepadAxis;
                sdlAxis = event->caxis.axis;
                sdlAxisValue = event->caxis.value;
                sdlAxisName = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)sdlAxis);
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

            // update controller button state
            for (auto& button : this->buttonWidgetMappings)
            {
                if (button.buttonWidget->HasInputData(inputType, sdlAxis, sdlAxisDirection))
                {
                    this->controllerImageWidget->SetButtonState(button.button, sdlAxisButtonPressed);
                }
            }

            // update controller analog stick state
            for (auto& joystick : this->joystickWidgetMappings)
            {
                if (joystick.buttonWidget->HasInputData(inputType, sdlAxis, sdlAxisDirection))
                {
                    const int value = -(double)((double)sdlAxisValue / SDL_AXIS_PEAK * 100);

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

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        { // keyboard button

            // make sure a keyboard is selected
            if (!this->isCurrentDeviceKeyboard())
            {
                break;
            }

            const SDL_Scancode sdlButton = (SDL_Scancode)event->key.keysym.scancode;
            const bool sdlButtonPressed = (event->type == SDL_KEYDOWN);

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
    return this->inputDeviceComboBox->currentData().toInt() != (int)InputDeviceType::None;
}

void ControllerWidget::SetSettingsSection(QString profile, QString section)
{
    this->settingsSection = section;
    this->profileComboBox->addItem(profile, section);
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
    CoreRomSettings romSettings;
    CoreRomHeader romHeader;
    if (CoreGetCurrentRomSettings(romSettings) &&
        CoreGetCurrentRomHeader(romHeader))
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
        if (CoreSettingsSectionExists(this->gameSection.toStdString()))
        {
            // check if the key 'UseGameProfile' exists,
            // if it doesn't then use the profile,
            // else check if it's true and use it
            if (!CoreSettingsKeyExists(this->gameSection.toStdString(), "UseGameProfile") ||
                CoreSettingsGetBoolValue(SettingsID::Input_UseGameProfile, this->gameSection.toStdString()))
            {
                this->profileComboBox->setCurrentText(internalName);
                section = this->gameSection;
            }
        }
    }

    // try to add all the user's profiles
    userProfiles = CoreSettingsGetStringListValue(SettingsID::Input_Profiles);
    for (const std::string& profile : userProfiles)
    {
        QString profileSection = this->getUserProfileSectionName(QString::fromStdString(profile));

        // ensure the profile section exists,
        // if it does, add it when it doesn't
        // exist yet
        if (CoreSettingsSectionExists(profileSection.toStdString()))
        {
            int index = this->profileComboBox->findData(profileSection);
            if (index == -1)
            {
                this->profileComboBox->addItem(QString::fromStdString(profile), profileSection);
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
    if (loadUserProfile && !useProfile.empty())
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

    this->deadZoneSlider->setValue(CoreSettingsGetIntValue(SettingsID::Input_Deadzone, section));
    this->optionsDialogSettings.RemoveDuplicateMappings = CoreSettingsGetBoolValue(SettingsID::Input_RemoveDuplicateMappings, section);
    this->optionsDialogSettings.ControllerPak = CoreSettingsGetIntValue(SettingsID::Input_Pak, section);
    this->optionsDialogSettings.GameboyRom = CoreSettingsGetStringValue(SettingsID::Input_GameboyRom, section);
    this->optionsDialogSettings.GameboySave = CoreSettingsGetStringValue(SettingsID::Input_GameboySave, section);

    // keep backwards compatibility with old profiles
    if (CoreSettingsKeyExists(section, "FilterEventsForButtons") &&
        CoreSettingsKeyExists(section, "FilterEventsForAxis"))
    {
        this->optionsDialogSettings.FilterEventsForButtons = CoreSettingsGetBoolValue(SettingsID::Input_FilterEventsForButtons, section);
        this->optionsDialogSettings.FilterEventsForAxis = CoreSettingsGetBoolValue(SettingsID::Input_FilterEventsForAxis, section);
    }
    else
    {
        this->optionsDialogSettings.FilterEventsForButtons = true;
        this->optionsDialogSettings.FilterEventsForAxis = true;
    }

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

    // force refresh some UI elements
    this->CheckInputDeviceSettings();
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
    CoreSettingsSetValue(SettingsID::Input_DeviceNum, section, (int)InputDeviceType::None);
    CoreSettingsSetValue(SettingsID::Input_Deadzone, section, 9);
    CoreSettingsSetValue(SettingsID::Input_Pak, section, 0);
    CoreSettingsSetValue(SettingsID::Input_RemoveDuplicateMappings, section, true);
    CoreSettingsSetValue(SettingsID::Input_FilterEventsForButtons, section, true);
    CoreSettingsSetValue(SettingsID::Input_FilterEventsForAxis, section, true);

    for (auto& buttonSetting : this->buttonSettingMappings)
    {
        CoreSettingsSetValue(buttonSetting.inputTypeSettingsId, section, std::vector<int>({ (int)InputType::Invalid }));
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

    this->SaveSettings(this->getCurrentSettingsSection());

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
    QString deviceName;
    int     deviceNum;

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

    this->GetCurrentInputDevice(deviceName, deviceNum, true);

    CoreSettingsSetValue(SettingsID::Input_PluggedIn, sectionStr, this->IsPluggedIn());
    CoreSettingsSetValue(SettingsID::Input_DeviceName, sectionStr, deviceName.toStdString());
    CoreSettingsSetValue(SettingsID::Input_DeviceNum, sectionStr, deviceNum);
    CoreSettingsSetValue(SettingsID::Input_Deadzone, sectionStr, this->deadZoneSlider->value());
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
}

void ControllerWidget::RevertSettings()
{
    QList<QString> sections;

    // add all sections from profile combobox
    for (int i = 0; i < this->profileComboBox->count(); i++)
    {
        sections.push_back(this->profileComboBox->itemData(i).toString());
    }

    // add section with profiles list
    sections.push_back("Rosalie's Mupen GUI - Input Plugin");

    // add removed profile sections
    for (QString section : this->removedProfiles)
    {
        // ensure it's unique
        if (!this->addedProfiles.contains(section) &&
            !sections.contains(section))
        {
            sections.push_back(section);
        }
    }

    // revert each section
    for (QString section : sections)
    {
        CoreSettingsRevertSection(section.toStdString());
    }

    // delete all sections for added profiles,
    // which weren't deleted
    for (QString section : this->addedProfiles)
    {
        if (!this->removedProfiles.contains(section))
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

