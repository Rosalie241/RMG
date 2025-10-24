/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "HotkeysDialog.hpp"

#include "UserInterface/UICommon.hpp"
#include "common.hpp"

using namespace UserInterface;

HotkeysDialog::HotkeysDialog(QWidget* parent, QList<HotkeySettingMapping> hotkeySettingMappings, 
    SDL_JoystickID joystickId, SDL_Gamepad* gamepad,
    bool filterEvents, bool removeDuplicates) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->currentJoystickId        = joystickId;
    this->currentGamepad           = gamepad;
    this->isCurrentJoystickGamepad = (gamepad != nullptr);
    this->filterEventsForButtons   = filterEvents;
    this->removeDuplicates         = removeDuplicates;

    this->hotkeySettingMappings.append(
    {
        { this->shutdownKeyButton, SettingsID::Input_Hotkey_Shutdown_InputType, SettingsID::Input_Hotkey_Shutdown_Name, SettingsID::Input_Hotkey_Shutdown_Data, SettingsID::Input_Hotkey_Shutdown_ExtraData },
        { this->exitKeyButton, SettingsID::Input_Hotkey_Exit_InputType, SettingsID::Input_Hotkey_Exit_Name, SettingsID::Input_Hotkey_Exit_Data, SettingsID::Input_Hotkey_Exit_ExtraData },
        { this->softResetKeyButton, SettingsID::Input_Hotkey_SoftReset_InputType, SettingsID::Input_Hotkey_SoftReset_Name, SettingsID::Input_Hotkey_SoftReset_Data, SettingsID::Input_Hotkey_SoftReset_ExtraData },
        { this->hardResetKeyButton, SettingsID::Input_Hotkey_HardReset_InputType, SettingsID::Input_Hotkey_HardReset_Name, SettingsID::Input_Hotkey_HardReset_Data, SettingsID::Input_Hotkey_HardReset_ExtraData },
        { this->pauseKeyButton, SettingsID::Input_Hotkey_Resume_InputType, SettingsID::Input_Hotkey_Resume_Name, SettingsID::Input_Hotkey_Resume_Data, SettingsID::Input_Hotkey_Resume_ExtraData },
        { this->generateBitmapKeyButton, SettingsID::Input_Hotkey_Screenshot_InputType, SettingsID::Input_Hotkey_Screenshot_Name, SettingsID::Input_Hotkey_Screenshot_Data, SettingsID::Input_Hotkey_Screenshot_ExtraData },
        { this->limitFPSKeyButton, SettingsID::Input_Hotkey_LimitFPS_InputType, SettingsID::Input_Hotkey_LimitFPS_Name, SettingsID::Input_Hotkey_LimitFPS_Data, SettingsID::Input_Hotkey_LimitFPS_ExtraData },
        { this->speedFactor25KeyButton, SettingsID::Input_Hotkey_SpeedFactor25_InputType, SettingsID::Input_Hotkey_SpeedFactor25_Name, SettingsID::Input_Hotkey_SpeedFactor25_Data, SettingsID::Input_Hotkey_SpeedFactor25_ExtraData },
        { this->speedFactor50KeyButton, SettingsID::Input_Hotkey_SpeedFactor50_InputType, SettingsID::Input_Hotkey_SpeedFactor50_Name, SettingsID::Input_Hotkey_SpeedFactor50_Data, SettingsID::Input_Hotkey_SpeedFactor50_ExtraData },
        { this->speedFactor75KeyButton, SettingsID::Input_Hotkey_SpeedFactor75_InputType, SettingsID::Input_Hotkey_SpeedFactor75_Name, SettingsID::Input_Hotkey_SpeedFactor75_Data, SettingsID::Input_Hotkey_SpeedFactor75_ExtraData },
        { this->speedFactor100KeyButton, SettingsID::Input_Hotkey_SpeedFactor100_InputType, SettingsID::Input_Hotkey_SpeedFactor100_Name, SettingsID::Input_Hotkey_SpeedFactor100_Data, SettingsID::Input_Hotkey_SpeedFactor100_ExtraData },
        { this->speedFactor125KeyButton, SettingsID::Input_Hotkey_SpeedFactor125_InputType, SettingsID::Input_Hotkey_SpeedFactor125_Name, SettingsID::Input_Hotkey_SpeedFactor125_Data, SettingsID::Input_Hotkey_SpeedFactor125_ExtraData },
        { this->speedFactor150KeyButton, SettingsID::Input_Hotkey_SpeedFactor150_InputType, SettingsID::Input_Hotkey_SpeedFactor150_Name, SettingsID::Input_Hotkey_SpeedFactor150_Data, SettingsID::Input_Hotkey_SpeedFactor150_ExtraData },
        { this->speedFactor175KeyButton, SettingsID::Input_Hotkey_SpeedFactor175_InputType, SettingsID::Input_Hotkey_SpeedFactor175_Name, SettingsID::Input_Hotkey_SpeedFactor175_Data, SettingsID::Input_Hotkey_SpeedFactor175_ExtraData },
        { this->speedFactor200KeyButton, SettingsID::Input_Hotkey_SpeedFactor200_InputType, SettingsID::Input_Hotkey_SpeedFactor200_Name, SettingsID::Input_Hotkey_SpeedFactor200_Data, SettingsID::Input_Hotkey_SpeedFactor200_ExtraData },
        { this->speedFactor225KeyButton, SettingsID::Input_Hotkey_SpeedFactor225_InputType, SettingsID::Input_Hotkey_SpeedFactor225_Name, SettingsID::Input_Hotkey_SpeedFactor225_Data, SettingsID::Input_Hotkey_SpeedFactor225_ExtraData },
        { this->speedFactor250KeyButton, SettingsID::Input_Hotkey_SpeedFactor250_InputType, SettingsID::Input_Hotkey_SpeedFactor250_Name, SettingsID::Input_Hotkey_SpeedFactor250_Data, SettingsID::Input_Hotkey_SpeedFactor250_ExtraData },
        { this->speedFactor275KeyButton, SettingsID::Input_Hotkey_SpeedFactor275_InputType, SettingsID::Input_Hotkey_SpeedFactor275_Name, SettingsID::Input_Hotkey_SpeedFactor275_Data, SettingsID::Input_Hotkey_SpeedFactor275_ExtraData },
        { this->speedFactor300KeyButton, SettingsID::Input_Hotkey_SpeedFactor300_InputType, SettingsID::Input_Hotkey_SpeedFactor300_Name, SettingsID::Input_Hotkey_SpeedFactor300_Data, SettingsID::Input_Hotkey_SpeedFactor300_ExtraData },
        { this->saveStateKeyButton, SettingsID::Input_Hotkey_SaveState_InputType, SettingsID::Input_Hotkey_SaveState_Name, SettingsID::Input_Hotkey_SaveState_Data, SettingsID::Input_Hotkey_SaveState_ExtraData },
        { this->loadStateKeyButton, SettingsID::Input_Hotkey_LoadState_InputType, SettingsID::Input_Hotkey_LoadState_Name, SettingsID::Input_Hotkey_LoadState_Data, SettingsID::Input_Hotkey_LoadState_ExtraData },
        { this->gsButtonKeyButton, SettingsID::Input_Hotkey_GSButton_InputType, SettingsID::Input_Hotkey_GSButton_Name, SettingsID::Input_Hotkey_GSButton_Data, SettingsID::Input_Hotkey_GSButton_ExtraData },
        { this->saveState0KeyButton, SettingsID::Input_Hotkey_SaveStateSlot0_InputType, SettingsID::Input_Hotkey_SaveStateSlot0_Name, SettingsID::Input_Hotkey_SaveStateSlot0_Data, SettingsID::Input_Hotkey_SaveStateSlot0_ExtraData },
        { this->saveState1KeyButton, SettingsID::Input_Hotkey_SaveStateSlot1_InputType, SettingsID::Input_Hotkey_SaveStateSlot1_Name, SettingsID::Input_Hotkey_SaveStateSlot1_Data, SettingsID::Input_Hotkey_SaveStateSlot1_ExtraData },
        { this->saveState2KeyButton, SettingsID::Input_Hotkey_SaveStateSlot2_InputType, SettingsID::Input_Hotkey_SaveStateSlot2_Name, SettingsID::Input_Hotkey_SaveStateSlot2_Data, SettingsID::Input_Hotkey_SaveStateSlot2_ExtraData },
        { this->saveState3KeyButton, SettingsID::Input_Hotkey_SaveStateSlot3_InputType, SettingsID::Input_Hotkey_SaveStateSlot3_Name, SettingsID::Input_Hotkey_SaveStateSlot3_Data, SettingsID::Input_Hotkey_SaveStateSlot3_ExtraData },
        { this->saveState4KeyButton, SettingsID::Input_Hotkey_SaveStateSlot4_InputType, SettingsID::Input_Hotkey_SaveStateSlot4_Name, SettingsID::Input_Hotkey_SaveStateSlot4_Data, SettingsID::Input_Hotkey_SaveStateSlot4_ExtraData },
        { this->saveState5KeyButton, SettingsID::Input_Hotkey_SaveStateSlot5_InputType, SettingsID::Input_Hotkey_SaveStateSlot5_Name, SettingsID::Input_Hotkey_SaveStateSlot5_Data, SettingsID::Input_Hotkey_SaveStateSlot5_ExtraData },
        { this->saveState6KeyButton, SettingsID::Input_Hotkey_SaveStateSlot6_InputType, SettingsID::Input_Hotkey_SaveStateSlot6_Name, SettingsID::Input_Hotkey_SaveStateSlot6_Data, SettingsID::Input_Hotkey_SaveStateSlot6_ExtraData },
        { this->saveState7KeyButton, SettingsID::Input_Hotkey_SaveStateSlot7_InputType, SettingsID::Input_Hotkey_SaveStateSlot7_Name, SettingsID::Input_Hotkey_SaveStateSlot7_Data, SettingsID::Input_Hotkey_SaveStateSlot7_ExtraData },
        { this->saveState8KeyButton, SettingsID::Input_Hotkey_SaveStateSlot8_InputType, SettingsID::Input_Hotkey_SaveStateSlot8_Name, SettingsID::Input_Hotkey_SaveStateSlot8_Data, SettingsID::Input_Hotkey_SaveStateSlot8_ExtraData },
        { this->saveState9KeyButton, SettingsID::Input_Hotkey_SaveStateSlot9_InputType, SettingsID::Input_Hotkey_SaveStateSlot9_Name, SettingsID::Input_Hotkey_SaveStateSlot9_Data, SettingsID::Input_Hotkey_SaveStateSlot9_ExtraData },
        { this->increaseSaveSlotKeyButton, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_InputType, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_Name, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_Data, SettingsID::Input_Hotkey_IncreaseSaveStateSlot_ExtraData },
        { this->decreaseSaveSlotKeyButton, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_InputType, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_Name, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_Data, SettingsID::Input_Hotkey_DecreaseSaveStateSlot_ExtraData },
        { this->memPakKeyButton, SettingsID::Input_Hotkey_MemoryPak_InputType, SettingsID::Input_Hotkey_MemoryPak_Name, SettingsID::Input_Hotkey_MemoryPak_Data, SettingsID::Input_Hotkey_MemoryPak_ExtraData },
        { this->rumblePakKeyButton, SettingsID::Input_Hotkey_RumblePak_InputType, SettingsID::Input_Hotkey_RumblePak_Name, SettingsID::Input_Hotkey_RumblePak_Data, SettingsID::Input_Hotkey_RumblePak_ExtraData },
        { this->noPakKeyButton, SettingsID::Input_Hotkey_NoPak_InputType, SettingsID::Input_Hotkey_NoPak_Name, SettingsID::Input_Hotkey_NoPak_Data, SettingsID::Input_Hotkey_NoPak_ExtraData },
        { this->fullscreenKeyButton, SettingsID::Input_Hotkey_Fullscreen_InputType, SettingsID::Input_Hotkey_Fullscreen_Name, SettingsID::Input_Hotkey_Fullscreen_Data, SettingsID::Input_Hotkey_Fullscreen_ExtraData },
    });

    // initialize buttons
    for (auto& mapping : this->hotkeySettingMappings)
    {
        mapping.button->Initialize(this);
    }

    // initialize icons
    this->infoIconLabel->setPixmap(QIcon::fromTheme("information-line").pixmap(16, 16));
    this->infoIconLabel_2->setPixmap(QIcon::fromTheme("information-line").pixmap(16, 16));
    this->infoIconLabel_3->setPixmap(QIcon::fromTheme("information-line").pixmap(16, 16));
    this->infoIconLabel_4->setPixmap(QIcon::fromTheme("information-line").pixmap(16, 16));
    this->infoIconLabel_5->setPixmap(QIcon::fromTheme("information-line").pixmap(16, 16));

    // load settings from given mappings
    for (qsizetype i = 0; i < hotkeySettingMappings.size(); i++)
    {
        auto& buttonMapping = this->hotkeySettingMappings.at(i);
        auto& givenMapping  = hotkeySettingMappings.at(i);

        for (size_t y = 0; y < givenMapping.inputTypes.size(); y++)
        {
            buttonMapping.button->AddInputData(
                static_cast<InputType>(givenMapping.inputTypes.at(y)),
                givenMapping.inputData.at(y),
                givenMapping.extraInputData.at(y),
                QString::fromStdString(givenMapping.inputText.at(y))
            );
        }

        buttonMapping.button->FinishState();
    }
}

void HotkeysDialog::on_MainDialog_SdlEvent(SDL_Event* event)
{
    switch (event->type)
    {
        default:
            break;

        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        {
            SDL_JoystickID joystickId = -1;
            InputType inputType = InputType::Invalid;
            int sdlButton = 0;
            QString sdlButtonName;

            if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            { // gamepad button
                if (!this->isCurrentJoystickGamepad &&
                    this->filterEventsForButtons)
                {
                    return;
                }

                joystickId = event->gbutton.which;
                inputType = InputType::GamepadButton;
                sdlButton = event->gbutton.button;
                sdlButtonName = UICommon::GetGamepadButtonText(this->currentGamepad, static_cast<SDL_GamepadButton>(sdlButton));
            }
            else
            { // joystick button
                if (this->isCurrentJoystickGamepad &&
                    this->filterEventsForButtons)
                {
                    return;
                }

                joystickId = event->jbutton.which;
                inputType = InputType::JoystickButton;
                sdlButton = event->jbutton.button;
                sdlButtonName = "button " + QString::number(sdlButton);
            }

            // make sure we have the right joystick
            if (joystickId != this->currentJoystickId)
            {
                break;
            }

            if (this->currentHotkeyButton != nullptr)
            {
                this->currentHotkeyButton->AddInputData(
                    inputType, 
                    sdlButton,
                    0,
                    sdlButtonName
                );
            }
        } break;

        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_JOYSTICK_BUTTON_UP:
        {
            SDL_JoystickID joystickId = -1;

            if (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP)
            { // gamepad button
                if (!this->isCurrentJoystickGamepad &&
                    this->filterEventsForButtons)
                {
                    return;
                }

                joystickId = event->gbutton.which;
            }
            else
            { // joystick button
                if (this->isCurrentJoystickGamepad &&
                    this->filterEventsForButtons)
                {
                    return;
                }

                joystickId = event->jbutton.which;
            }

            // make sure we have the right joystick
            if (joystickId != this->currentJoystickId)
            {
                break;
            }

            if (this->currentHotkeyButton != nullptr)
            {
                this->currentHotkeyButton->StopTimer();
                this->currentHotkeyButton->FinishState();
                this->currentHotkeyButton->clearFocus();
                this->currentHotkeyButton = nullptr;
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
                if (!this->isCurrentJoystickGamepad &&
                    this->filterEventsForButtons)
                {
                    return;
                }

                joystickId = event->gaxis.which;
                inputType = InputType::GamepadAxis;
                sdlAxis = event->gaxis.axis;
                sdlAxisValue = event->gaxis.value;
                sdlAxisName = SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(sdlAxis));
                sdlAxisName += sdlAxisValue > 0 ? "+" : "-";
            }
            else
            { // joystick axis
                if (this->isCurrentJoystickGamepad &&
                    this->filterEventsForButtons)
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
            const int sdlAxisDirection = (sdlAxisValue > 0 ? 1 : 0);

            // make sure we have the right joystick
            if (joystickId != this->currentJoystickId)
            {
                break;
            }

            if (this->currentHotkeyButton != nullptr)
            {
                // when the axis has been pressed for more
                // than 50%, add it to the hotkey, if it isn't
                // but if the current hotkey button contains
                // that axis, then we can stop watching
                // for input
                if (sdlAxisButtonPressed)
                {
                    this->currentHotkeyButton->AddInputData(
                        inputType, 
                        sdlAxis,
                        sdlAxisDirection,
                        sdlAxisName
                    );
                }
                else if (this->currentHotkeyButton->HasInputData(inputType, sdlAxis, sdlAxisDirection))
                {
                    this->currentHotkeyButton->StopTimer();
                    this->currentHotkeyButton->FinishState();
                    this->currentHotkeyButton->clearFocus();
                    this->currentHotkeyButton = nullptr;
                }
            }
        } break;
    }
}

void HotkeysDialog::on_HotkeyButton_Released(Widget::HotkeyButton* button)
{
    if (this->currentHotkeyButton != nullptr)
    {
        this->currentHotkeyButton->StopTimer();
        this->currentHotkeyButton->RestoreState();
    }

    this->currentHotkeyButton = button;

    button->SaveState();
    button->StartTimer();
}

void HotkeysDialog::on_HotkeyButton_TimerFinished(Widget::HotkeyButton* button)
{
    if (this->currentHotkeyButton == button)
    {
        this->currentHotkeyButton = nullptr;
    }

    button->RestoreState();
    button->clearFocus();
}

void HotkeysDialog::on_HotkeyButton_StateFinished(Widget::HotkeyButton* button)
{
    // do nothing when the user
    // doesn't want us to remove 
    // duplicates
    if (!this->removeDuplicates)
    {
        return;
    }

    std::vector<int> inputType = button->GetInputType();
    std::vector<int> inputData = button->GetInputData();
    std::vector<int> extraInputData = button->GetExtraInputData();

    for (auto& mapping : this->hotkeySettingMappings)
    {
        // skip ourselves
        if (mapping.button == button)
        {
            continue;
        }

        if (inputType == mapping.button->GetInputType() &&
            inputData == mapping.button->GetInputData() &&
            extraInputData == mapping.button->GetExtraInputData())
        {
            mapping.button->Clear();
        }
    }
}

QList<HotkeySettingMapping> HotkeysDialog::GetSettingMappings()
{
    QList<HotkeySettingMapping> mappings;

    for (auto& mapping : this->hotkeySettingMappings)
    {
        mappings.append({ 
            mapping.button->GetInputType(),
            mapping.button->GetInputData(),
            mapping.button->GetExtraInputData(),
            mapping.button->GetInputText(),
            mapping.inputTypeSettingsId,
            mapping.nameSettingsId,
            mapping.dataSettingsId,
            mapping.extraDataSettingsId
        });
    }

    return mappings;
}
