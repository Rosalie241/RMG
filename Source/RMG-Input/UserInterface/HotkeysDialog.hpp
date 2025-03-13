/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef HOTKEYSDIALOG_HPP
#define HOTKEYSDIALOG_HPP

#include <QDialog>
#include <string>

#include <SDL.h>

#include <RMG-Core/Settings.hpp>

#include "ui_HotkeysDialog.h"

namespace UserInterface
{

struct HotkeySettingMapping
{
    std::vector<int> inputTypes;
    std::vector<int> inputData;
    std::vector<int> extraInputData;
    std::vector<std::string> inputText;
    SettingsID inputTypeSettingsId;
    SettingsID nameSettingsId;
    SettingsID dataSettingsId;
    SettingsID extraDataSettingsId;
};

class HotkeysDialog : public QDialog, private Ui::HotkeysDialog
{
Q_OBJECT

private:

    SDL_JoystickID currentJoystickId     = -1;
    bool isCurrentJoystickGameController = false;
    bool filterEventsForButtons          = false;
    bool removeDuplicates                = false;

    struct hotkeySettingMapping
    {
        Widget::HotkeyButton* button;
        SettingsID inputTypeSettingsId;
        SettingsID nameSettingsId;
        SettingsID dataSettingsId;
        SettingsID extraDataSettingsId;
    };

    QList<hotkeySettingMapping> hotkeySettingMappings;

    Widget::HotkeyButton* currentHotkeyButton = nullptr;

public:
    HotkeysDialog(QWidget *parent, QList<HotkeySettingMapping> hotkeySettingMappings, 
        bool isGameController, SDL_JoystickID joystickId, 
        bool filterEvents, bool removeDuplicates);

    QList<HotkeySettingMapping> GetSettingMappings();

public slots:
    void on_MainDialog_SdlEvent(SDL_Event* event);

    void on_HotkeyButton_Released(Widget::HotkeyButton* button);
    void on_HotkeyButton_TimerFinished(Widget::HotkeyButton* button);
    void on_HotkeyButton_StateFinished(Widget::HotkeyButton* button);
};
}

#endif // HOTKEYSDIALOG_HPP
