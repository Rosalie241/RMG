/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "Utilities/SettingsID.hpp"
#include <QString>

namespace Utilities
{
class Settings
{
  public:
    Settings();
    ~Settings();

    void LoadDefaults(void);

    int GetDefaultIntValue(SettingsID);
    bool GetDefaultBoolValue(SettingsID);
    float GetDefaultFloatValue(SettingsID);
    QString GetDefaultStringValue(SettingsID);

    int GetIntValue(SettingsID);
    bool GetBoolValue(SettingsID);
    float GetFloatValue(SettingsID);
    QString GetStringValue(SettingsID);

    bool SetValue(SettingsID, int);
    bool SetValue(SettingsID, bool);
    bool SetValue(SettingsID, float);
    bool SetValue(SettingsID, QString);

  private:
    Setting_t getSetting(SettingsID);
};
} // namespace Utilities

#endif // SETTINGS_HPP