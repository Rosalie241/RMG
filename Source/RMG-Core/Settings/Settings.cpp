/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Settings.hpp"

#include "m64p/Api.hpp"
#include "Error.hpp"
#include "m64p/api/m64p_types.h"

#include <exception>
#include <string>
#include <sstream>
#include <algorithm>

//
// Local Defines
//

#define STR_SIZE 4096

//
// Local Structures
//

struct l_DynamicValue
{
    int intValue;
    bool boolValue;
    float floatValue;
    std::string stringValue;
    std::vector<int> intListValue;

    m64p_type valueType;

    l_DynamicValue() {}

    l_DynamicValue(int value)
    {
        intValue = value;
        valueType = M64TYPE_INT;
    }

    l_DynamicValue(bool value)
    {
        boolValue = value;
        valueType = M64TYPE_BOOL;
    }

    l_DynamicValue(float value)
    {
        floatValue = value;
        valueType = M64TYPE_FLOAT;
    }

    l_DynamicValue(const char* value)
    {
        stringValue = std::string(value);
        valueType = M64TYPE_STRING;
    }

    l_DynamicValue(std::vector<int> value)
    {
        intListValue = value;
        // convert list to string
        std::string value_str;
        for (int num : value)
        {
            value_str += std::to_string(num);
            value_str += ";";
        }
        stringValue = value_str;
        valueType = M64TYPE_STRING;
    }
};

typedef struct
{
    std::string Section;
    std::string Key;
    l_DynamicValue DefaultValue;
    std::string Description;
    bool ForceUseSet;
} l_Setting;

//
// Local Variables
//

static m64p_handle              l_sectionHandle = nullptr;
static std::vector<std::string> l_sectionList;

//
// Local Functions
//

#define SETTING_SECTION_GUI         "Rosalie's Mupen GUI"
#define SETTING_SECTION_CORE        SETTING_SECTION_GUI  " Core"
#define SETTING_SECTION_OVERLAY     SETTING_SECTION_CORE " Overlay"
#define SETTING_SECTION_KEYBIND     SETTING_SECTION_GUI  " KeyBindings"
#define SETTING_SECTION_ROMBROWSER  SETTING_SECTION_GUI  " RomBrowser"
#define SETTING_SECTION_M64P        "Core"

// retrieves l_Setting from settingId
static l_Setting get_setting(SettingsID settingId)
{
    l_Setting setting;

    switch (settingId)
    {
    default:
        setting = {"", "", "", "", false};
        break;

    case SettingsID::GUI_SettingsDialogWidth:
        setting = {SETTING_SECTION_GUI, "SettingsDialogWidth", 0, "", false};
        break;
    case SettingsID::GUI_SettingsDialogHeight:
        setting = {SETTING_SECTION_GUI, "SettingsDialogHeight", 0, "", false};
        break;
    case SettingsID::GUI_AllowManualResizing:
        setting = {SETTING_SECTION_GUI, "AllowManualResizing", true, "", false};
        break;
    case SettingsID::GUI_HideCursorInEmulation:
        setting = {SETTING_SECTION_GUI, "HideCursorInEmulation", false, "", false};
        break;
    case SettingsID::GUI_StatusbarMessageDuration:
        setting = {SETTING_SECTION_GUI, "StatusbarMessageDuration", 3, "", false};
        break;

    case SettingsID::Core_GFX_Plugin:
        setting = {SETTING_SECTION_CORE, "GFX_Plugin", "", "", false};
        break;
    case SettingsID::Core_AUDIO_Plugin:
        setting = {SETTING_SECTION_CORE, "AUDIO_Plugin", "", "", false};
        break;
    case SettingsID::Core_INPUT_Plugin:
        setting = {SETTING_SECTION_CORE, "INPUT_Plugin", "", "", false};
        break;
    case SettingsID::Core_RSP_Plugin:
        setting = {SETTING_SECTION_CORE, "RSP_Plugin", "", "", false};
        break;

    case SettingsID::Core_OverrideUserDirs:
        setting = {SETTING_SECTION_CORE, "OverrideUserDirectories", true, "", false};
        break;
    case SettingsID::Core_UserDataDirOverride:
        setting = {SETTING_SECTION_CORE, "UserDataDirectory", "Data", "", false};
        break;
    case SettingsID::Core_UserCacheDirOverride:
        setting = {SETTING_SECTION_CORE, "UserCacheDirectory", "Cache", "", false};
        break;

    case SettingsID::Core_OverrideGameSpecificSettings:
        setting = {SETTING_SECTION_CORE, "OverrideGameSpecificSettings", false, "", false};
        break;

    case SettingsID::Core_RandomizeInterrupt:
        setting = {SETTING_SECTION_OVERLAY, "RandomizeInterrupt", true, "", false};
        break;
    case SettingsID::Core_CPU_Emulator:
        setting = {SETTING_SECTION_OVERLAY, "CPU_Emulator", 2, "", false};
        break;
    case SettingsID::Core_DisableExtraMem:
        setting = {SETTING_SECTION_OVERLAY, "DisableExtraMem", false, "", false};
        break;
    case SettingsID::Core_EnableDebugger:
        setting = {SETTING_SECTION_OVERLAY, "EnableDebugger", false, "", false};
        break;
    case SettingsID::Core_CountPerOp:
        setting = {SETTING_SECTION_OVERLAY, "CountPerOp", 0, "", false};
        break;
    case SettingsID::Core_SiDmaDuration:
        setting = {SETTING_SECTION_OVERLAY, "SiDmaDuration", -1, "", false};
        break;

    case SettingsID::Core_ScreenshotPath:
        setting = {SETTING_SECTION_M64P, "ScreenshotPath", "Screenshots", "", true};
        break;
    case SettingsID::Core_SaveStatePath:
        setting = {SETTING_SECTION_M64P, "SaveStatePath", "Save/State", "", true};
        break;
    case SettingsID::Core_SaveSRAMPath:
        setting = {SETTING_SECTION_M64P, "SaveSRAMPath", "Save/Game", "", true};
        break;
    case SettingsID::Core_SharedDataPath:
        setting = {SETTING_SECTION_M64P, "SharedDataPath", "Data", "", true};
        break;

    case SettingsID::Core_64DD_RomFile:
        setting = {SETTING_SECTION_CORE, "64DD_RomFile", "", "", false};
        break;

    case SettingsID::Game_DisableExtraMem:
        setting = {"", "DisableExtraMem", false, "", false};
        break;
    case SettingsID::Game_SaveType:
        setting = {"", "SaveType", 0, "", false};
        break;
    case SettingsID::Game_CountPerOp:
        setting = {"", "CountPerOp", 2, "", false};
        break;
    case SettingsID::Game_SiDmaDuration:
        setting = {"", "SiDmaDuration", 2304, "", false};
        break;

    case SettingsID::Game_OverrideCoreSettings:
        setting = {"", "OverrideCoreSettings", false, "", false};
        break;
    case SettingsID::Game_CPU_Emulator:
        setting = {"", "CPU_Emulator", 2, "", false};
        break;
    case SettingsID::Game_RandomizeInterrupt:
        setting = {"", "RandomizeInterrupt", true, "", false};
        break;

    case SettingsID::Game_GFX_Plugin:
        setting = {"", "GFX_Plugin", "", "", false};
        break;
    case SettingsID::Game_AUDIO_Plugin:
        setting = {"", "AUDIO_Plugin", "", "", false};
        break;
    case SettingsID::Game_INPUT_Plugin:
        setting = {"", "INPUT_Plugin", "", "", false};
        break;
    case SettingsID::Game_RSP_Plugin:
        setting = {"", "RSP_Plugin", "", "", false};
        break;

    case SettingsID::KeyBinding_OpenROM:
        setting = {SETTING_SECTION_KEYBIND, "OpenROM", "Ctrl+O", "", false};
        break;
    case SettingsID::KeyBinding_OpenCombo:
        setting = {SETTING_SECTION_KEYBIND, "OpenCombo", "Ctrl+Shift+O", "", false};
        break;
    case SettingsID::KeyBinding_StartEmulation:
        setting = {SETTING_SECTION_KEYBIND, "StartEmulation", "F11", "", false};
        break;
    case SettingsID::KeyBinding_EndEmulation:
        setting = {SETTING_SECTION_KEYBIND, "EndEmulation", "F12", "", false};
        break;
    case SettingsID::KeyBinding_RefreshROMList:
        setting = {SETTING_SECTION_KEYBIND, "RefreshROMList", "F5", "", false};
        break;
    case SettingsID::KeyBinding_Exit:
        setting = {SETTING_SECTION_KEYBIND, "Exit", "Alt+F4", "", false};
        break;
    case SettingsID::KeyBinding_SoftReset:
        setting = {SETTING_SECTION_KEYBIND, "SoftReset", "F1", "", false};
        break;
    case SettingsID::KeyBinding_HardReset:
        setting = {SETTING_SECTION_KEYBIND, "HardReset", "Shift+F1", "", false};
        break;
    case SettingsID::KeyBinding_Resume:
        setting = {SETTING_SECTION_KEYBIND, "Resume", "F2", "", false};
        break;
    case SettingsID::KeyBinding_GenerateBitmap:
        setting = {SETTING_SECTION_KEYBIND, "GenerateBitmap", "F3", "", false};
        break;
    case SettingsID::KeyBinding_LimitFPS:
        setting = {SETTING_SECTION_KEYBIND, "LimitFPS", "F4", "", false};
        break;
    case SettingsID::KeyBinding_SwapDisk:
        setting = {SETTING_SECTION_KEYBIND, "SwapDisk", "Ctrl+D", "", false};
        break;
    case SettingsID::KeyBinding_SaveState:
        setting = {SETTING_SECTION_KEYBIND, "SaveState", "F5", "", false};
        break;
    case SettingsID::KeyBinding_SaveAs:
        setting = {SETTING_SECTION_KEYBIND, "SaveAs", "Ctrl+S", "", false};
        break;
    case SettingsID::KeyBinding_LoadState:
        setting = {SETTING_SECTION_KEYBIND, "LoadState", "F7", "", false};
        break;
    case SettingsID::KeyBinding_Load:
        setting = {SETTING_SECTION_KEYBIND, "Load", "Ctrl+L", "", false};
        break;
    case SettingsID::KeyBinding_Cheats:
        setting = {SETTING_SECTION_KEYBIND, "Cheats", "Ctrl+C", "", false};
        break;
    case SettingsID::KeyBinding_GSButton:
        setting = {SETTING_SECTION_KEYBIND, "GSButton", "F9", "", false};
        break;
    case SettingsID::KeyBinding_Fullscreen:
        setting = {SETTING_SECTION_KEYBIND, "Fullscreen", "Alt+Return", "", false};
        break;
    case SettingsID::KeyBinding_Settings:
        setting = {SETTING_SECTION_KEYBIND, "Settings", "Ctrl+T", "", false};
        break;

    case SettingsID::RomBrowser_Directory:
        setting = {SETTING_SECTION_ROMBROWSER, "Directory", "", "", false};
        break;
    case SettingsID::RomBrowser_Geometry:
        setting = {SETTING_SECTION_ROMBROWSER, "Geometry", "", "", false};
        break;
    case SettingsID::RomBrowser_Recursive:
        setting = {SETTING_SECTION_ROMBROWSER, "Recursive", true, "", false};
        break;
    case SettingsID::RomBrowser_MaxItems:
        setting = {SETTING_SECTION_ROMBROWSER, "MaxItems", 50, "", false};
        break;
    case SettingsID::RomBrowser_Columns:
        setting = {SETTING_SECTION_ROMBROWSER, "Columns", std::vector<int>({0, 1, 2}), "", false};
        break;
    case SettingsID::RomBrowser_ColumnSizes:
        setting = {SETTING_SECTION_ROMBROWSER, "ColumnSizes", std::vector<int>({0, 250, 1, 100, 2, 100}), "", false};
        break;
    }

    return setting;
}

static void config_listsections_callback(void* context, const char* section)
{
    l_sectionList.emplace_back(std::string(section));
}

static bool config_section_exists(std::string section)
{
    std::string error;
    m64p_error ret;

    l_sectionList.clear();

    ret = m64p::Config.ListSections(nullptr, &config_listsections_callback);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_section_exists m64p::Config.ListSections Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        return false;
    }

    return std::find(l_sectionList.begin(), l_sectionList.end(), section) != l_sectionList.end();
}

static bool config_section_open(std::string section)
{
    std::string error;
    m64p_error ret;

    if (section.empty())
    {
        error = "config_section_open Failed: cannot open empty section!";
        CoreSetError(error);
        return false;
    }

    ret = m64p::Config.OpenSection(section.c_str(), &l_sectionHandle);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_section_open Failed: ";
        error = m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static bool config_option_set(std::string section, std::string key, m64p_type type, void *value)
{
    std::string error;
    m64p_error ret;

    if (!config_section_open(section))
    {
        return false;
    }

    ret = m64p::Config.SetParameter(l_sectionHandle, key.c_str(), type, value);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_option_set m64p::Config.SetParameter Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static bool config_option_get(std::string section, std::string key, m64p_type type, void *value, int size)
{
    std::string error;
    m64p_error ret;

    if (!config_section_exists(section))
    {
        error = "config_option_get Failed: cannot open non-existent section!";
        CoreSetError(error);
        return false;
    }

    if (!config_section_open(section))
    {
        return false;
    }

    ret = m64p::Config.GetParameter(l_sectionHandle, key.c_str(), type, value, size);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_option_get m64p::Config.GetParameter Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static bool config_option_default_set(std::string section, std::string key, m64p_type type, void *value, const char* description)
{
    std::string error;
    m64p_error ret;

    if (!config_section_open(section))
    {
        return false;
    }

    switch (type)
    {
        default:
        {
            ret = M64ERR_INPUT_INVALID;
            error = "config_option_default_set Failed: invalid type parameter!";
        } break;
        case M64TYPE_INT:
        {
            ret = m64p::Config.SetDefaultInt(l_sectionHandle, key.c_str(), *(int*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultInt Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
        case M64TYPE_BOOL:
        {
            ret = m64p::Config.SetDefaultBool(l_sectionHandle, key.c_str(), *(bool*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultBool Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
        case M64TYPE_FLOAT:
        {
            ret = m64p::Config.SetDefaultFloat(l_sectionHandle, key.c_str(), *(float*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultFloat Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
        case M64TYPE_STRING:
        {
            ret = m64p::Config.SetDefaultString(l_sectionHandle, key.c_str(), (char*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultString Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
    }

    if (ret != M64ERR_SUCCESS)
    {
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

//
// Exported Functions
//

bool CoreSettingsSave(void)
{
    std::string error;
    m64p_error ret;

    ret = m64p::Config.SaveFile();
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSettingsSave m64p::Config.SaveFile Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreSettingsSetupDefaults(void)
{
    l_Setting setting;
    bool ret;

    for (int i = 0; i < (int)SettingsID::Invalid; i++)
    {
        setting = get_setting((SettingsID)i);

        if (setting.Section.empty())
        {
            continue;
        }

        switch (setting.DefaultValue.valueType)
        {
        case M64TYPE_STRING:
        {
            if (setting.ForceUseSet)
            {
                ret = config_option_set(setting.Section, setting.Key, M64TYPE_STRING, (void*)setting.DefaultValue.stringValue.c_str());
            }
            else
            {
                ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_STRING, (void*)setting.DefaultValue.stringValue.c_str(), setting.Description.c_str());
            }
        } break;
        case M64TYPE_INT:
            ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_INT, &setting.DefaultValue.intValue, setting.Description.c_str());
            break;
        case M64TYPE_BOOL:
            ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_BOOL, &setting.DefaultValue.boolValue, setting.Description.c_str());
            break;
        case M64TYPE_FLOAT:
            ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_FLOAT, &setting.DefaultValue.intValue, setting.Description.c_str());
            break;
        }

        if (!ret)
        {
            return false;
        }
    }

    return true;
}

bool CoreSettingsSetValue(SettingsID settingId, int value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(setting.Section, setting.Key, M64TYPE_INT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, bool value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(setting.Section, setting.Key, M64TYPE_BOOL, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, float value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(setting.Section, setting.Key, M64TYPE_FLOAT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(setting.Section, setting.Key, M64TYPE_STRING, (void*)value.c_str());
}

bool CoreSettingsSetValue(SettingsID settingId, std::vector<int> value)
{
    std::string value_str;
    for (const int& num : value)
    {
        value_str += std::to_string(num);
        value_str += ";";
    }
    return CoreSettingsSetValue(settingId, value_str);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, int value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(section, setting.Key, M64TYPE_INT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, bool value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(section, setting.Key, M64TYPE_BOOL, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, float value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(section, setting.Key, M64TYPE_FLOAT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::string value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(section, setting.Key, M64TYPE_STRING, (void*)value.c_str());
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::vector<int> value)
{
    std::string value_str;
    for (const int& num : value)
    {
        value_str += std::to_string(num);
        value_str += ";";
    }
    return CoreSettingsSetValue(settingId, section, value_str);
}

int CoreSettingsGetDefaultIntValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.intValue;
}

bool CoreSettingsGetDefaultBoolValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.boolValue;
}

float CoreSettingsGetDefaultFloatValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.floatValue;
}

std::string CoreSettingsGetDefaultStringValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.stringValue;
}

std::vector<int> CoreSettingsGetDefaultIntListValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.intListValue;
}

int CoreSettingsGetIntValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.intValue;
    config_option_get(setting.Section, setting.Key, M64TYPE_INT, &value, sizeof(value));
    return value;
}

bool CoreSettingsGetBoolValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.boolValue;
    config_option_get(setting.Section, setting.Key, M64TYPE_BOOL, &value, sizeof(value));
    return value;
}

float CoreSettingsGetFloatValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    float value = setting.DefaultValue.floatValue;
    config_option_get(setting.Section, setting.Key, M64TYPE_FLOAT, &value, sizeof(value));
    return value;
}

std::string CoreSettingsGetStringValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    char value[STR_SIZE] = {0};
    config_option_get(setting.Section, setting.Key, M64TYPE_STRING, (char*)value, sizeof(value));
    return std::string(value);
}

std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return CoreSettingsGetIntListValue(settingId, setting.Section);
}

int CoreSettingsGetIntValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.intValue;
    config_option_get(section, setting.Key, M64TYPE_INT, &value, sizeof(value));
    return value;
}

bool CoreSettingsGetBoolValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.boolValue;
    config_option_get(section, setting.Key, M64TYPE_BOOL, &value, sizeof(value));
    return value;
}

float CoreSettingsGetFloatValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    float value = setting.DefaultValue.floatValue;
    config_option_get(section, setting.Key, M64TYPE_FLOAT, &value, sizeof(value));
    return value;
}

std::string CoreSettingsGetStringValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    char value[STR_SIZE] = {0};
    config_option_get(section, setting.Key, M64TYPE_STRING, (char*)value, sizeof(value));
    return std::string(value);
}

std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    std::vector<int> value;

    std::string value_str;
    value_str = CoreSettingsGetStringValue(settingId, section);

    // split string by ';'
    // and append list with each item
    std::stringstream value_str_stream(value_str);
    std::string tmp_str;
    while (std::getline(value_str_stream, tmp_str, ';'))
    {
        try
        {
            value.emplace_back(std::stoi(tmp_str));
        }
        catch (std::exception)
        { // ignore exception
            continue;
        }
    }

    return value;
}
