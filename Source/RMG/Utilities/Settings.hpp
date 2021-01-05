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
    QList<int> GetDefaultIntListValue(SettingsID);

    int GetDefaultIntValue(SettingsID, QString);
    bool GetDefaultBoolValue(SettingsID, QString);
    float GetDefaultFloatValue(SettingsID, QString);
    QString GetDefaultStringValue(SettingsID, QString);
    QList<int> GetDefaultIntListValue(SettingsID, QString);

    int GetIntValue(SettingsID);
    bool GetBoolValue(SettingsID);
    float GetFloatValue(SettingsID);
    QString GetStringValue(SettingsID);
    QList<int> GetIntListValue(SettingsID);

    int GetIntValue(SettingsID, QString);
    bool GetBoolValue(SettingsID, QString);
    float GetFloatValue(SettingsID, QString);
    QString GetStringValue(SettingsID, QString);
    QList<int> GetIntListValue(SettingsID, QString);

    bool SetValue(SettingsID, int);
    bool SetValue(SettingsID, bool);
    bool SetValue(SettingsID, float);
    bool SetValue(SettingsID, QString);
    bool SetValue(SettingsID, QList<int>);

    bool SetValue(SettingsID, QString, int);
    bool SetValue(SettingsID, QString, bool);
    bool SetValue(SettingsID, QString, float);
    bool SetValue(SettingsID, QString, QString);
    bool SetValue(SettingsID, QString, QList<int>);

  private:
    Setting_t getSetting(SettingsID);

    int getDefaultIntValue(Setting_t);
    bool getDefaultBoolValue(Setting_t);
    float getDefaultFloatValue(Setting_t);
    QString getDefaultStringValue(Setting_t);
    QList<int> getDefaultIntListValue(Setting_t);

    int getIntValue(Setting_t);
    bool getBoolValue(Setting_t);
    float getFloatValue(Setting_t);
    QString getStringValue(Setting_t);
    QList<int> getIntListValue(Setting_t);

    bool setValue(Setting_t, int);
    bool setValue(Setting_t, bool);
    bool setValue(Setting_t, float);
    bool setValue(Setting_t, QString);
    bool setValue(Setting_t, QList<int>);
};
} // namespace Utilities

#endif // SETTINGS_HPP