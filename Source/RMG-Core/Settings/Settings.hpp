/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_SETTINGS_HPP
#define CORE_SETTINGS_HPP

#include "SettingsID.hpp"

#include <string>
#include <vector>

// saves settings to file
bool CoreSettingsSave(void);

// upgrades existing settings to new version
bool CoreSettingsUpgrade(void);

// setup default settings
bool CoreSettingsSetupDefaults(void);

// returns whether a section exists
bool CoreSettingsSectionExists(std::string section);

// deletes given section
bool CoreSettingsDeleteSection(std::string section);

// returns whether a key in the given section exists
bool CoreSettingsKeyExists(std::string section, std::string key);

// sets setting as int value
bool CoreSettingsSetValue(SettingsID settingId, int value);
// sets setting as bool value
bool CoreSettingsSetValue(SettingsID settingId, bool value);
// sets setting as float value
bool CoreSettingsSetValue(SettingsID settingId, float value);
// sets setting as string value
bool CoreSettingsSetValue(SettingsID settingId, std::string value);
// sets setting as int list value
bool CoreSettingsSetValue(SettingsID settingId, std::vector<int> value);

// sets setting in section as int value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, int value);
// sets setting in section as bool value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, bool value);
// sets setting in section as float value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, float value);
// sets setting in section as string value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::string value);
// sets setting in section as int list value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::vector<int> value);

#ifdef CORE_INTERNAL
// sets key in section as int value
bool CoreSettingsSetValue(std::string section, std::string key, int value);
// sets key in section as bool value
bool CoreSettingsSetValue(std::string section, std::string key, bool value);
// sets key in section as float value
bool CoreSettingsSetValue(std::string section, std::string key, float value);
// sets key in section as string value
bool CoreSettingsSetValue(std::string section, std::string key, std::string value);
// sets key in section as int list value
bool CoreSettingsSetValue(std::string section, std::string key, std::vector<int> value);
#endif // CORE_INTERNAL

// retrieves default setting as int
int CoreSettingsGetDefaultIntValue(SettingsID settingId);
// retrieves default setting as bool
bool CoreSettingsGetDefaultBoolValue(SettingsID settingId);
// retrieves default setting as float
float CoreSettingsGetDefaultFloatValue(SettingsID settingId);
// retrieves default setting as string
std::string CoreSettingsGetDefaultStringValue(SettingsID settingId);
// retrieves default setting as int list
std::vector<int> CoreSettingsGetDefaultIntListValue(SettingsID settingId);

// retrieves setting as int
int CoreSettingsGetIntValue(SettingsID settingId);
// retrieves setting as bool
bool CoreSettingsGetBoolValue(SettingsID settingId);
// retrieves setting as float
float CoreSettingsGetFloatValue(SettingsID settingId);
// retrieves setting as string
std::string CoreSettingsGetStringValue(SettingsID settingId);
// retrieves setting as int list
std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId);

// retrieves setting in section as int
int CoreSettingsGetIntValue(SettingsID settingId, std::string section);
// retrieves setting in section as bool
bool CoreSettingsGetBoolValue(SettingsID settingId, std::string section);
// retrieves setting in section as float
float CoreSettingsGetFloatValue(SettingsID settingId, std::string section);
// retrieves setting in section as string
std::string CoreSettingsGetStringValue(SettingsID settingId, std::string section);
// retrieves setting in section as int list
std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId, std::string section);

#ifdef CORE_INTERNAL
// retrieves key in section as int
int CoreSettingsGetIntValue(std::string section, std::string key, int defaultValue);
// retrieves key in section as bool
bool CoreSettingsGetBoolValue(std::string section, std::string key, bool defaultValue);
// retrieves key in section as float
float CoreSettingsGetFloatValue(std::string section, std::string key, float defaultValue);
// retrieves key in section as string
std::string CoreSettingsGetStringValue(std::string section, std::string key);
// retrieves key in section as int list
std::vector<int> CoreSettingsGetIntListValue(std::string section, std::string key);
#endif // CORE_INTERNAL

#endif // CORE_SETTINGS_HPP
