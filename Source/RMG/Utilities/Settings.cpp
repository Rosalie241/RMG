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
#include "../Globals.hpp"
#include "Utilities/SettingsID.hpp"
#include <qopenglext.h>
#include <qvariant.h>

using namespace Utilities;

Settings::Settings()
{
}

Settings::~Settings()
{
}

void Settings::LoadDefaults()
{
    Setting_t setting;
    for (int i = 0; i < SettingsID::Invalid; i++)
    {
        setting = this->getSetting((SettingsID)i);

        switch (setting.Default.type())
        {
        case QVariant::Type::String:
            if (setting.ForceUseSet && this->GetStringValue((SettingsID)i).isEmpty())
                this->SetValue((SettingsID)i, setting.Default.toString());
            else
                g_MupenApi.Config.SetDefaultOption(setting.Section, setting.Key, setting.Default.toString(),
                                                   setting.Help);
            break;
        case QVariant::Type::Int:
            g_MupenApi.Config.SetDefaultOption(setting.Section, setting.Key, setting.Default.toInt(), setting.Help);
            break;
        default:
        case QVariant::Type::Bool:
            g_MupenApi.Config.SetDefaultOption(setting.Section, setting.Key, setting.Default.toBool(), setting.Help);
            break;
        }
    }

    g_Plugins.LoadSettings();

    g_MupenApi.Config.Save();
}

int Settings::GetDefaultIntValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    return s.Default.toInt();
}

bool Settings::GetDefaultBoolValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    return s.Default.toBool();
}

float Settings::GetDefaultFloatValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    return s.Default.toFloat();
}

QString Settings::GetDefaultStringValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    return s.Default.toString();
}

int Settings::GetIntValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    int value = s.Default.toInt();
    g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

bool Settings::GetBoolValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    bool value = s.Default.toBool();
    g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

float Settings::GetFloatValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    float value = s.Default.toFloat();
    g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

QString Settings::GetStringValue(SettingsID id)
{
    Setting_t s = this->getSetting(id);
    QString value = s.Default.toString();
    g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

bool Settings::SetValue(SettingsID id, int value)
{
    Setting_t s = this->getSetting(id);
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

bool Settings::SetValue(SettingsID id, bool value)
{
    Setting_t s = this->getSetting(id);
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

bool Settings::SetValue(SettingsID id, float value)
{
    Setting_t s = this->getSetting(id);
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

bool Settings::SetValue(SettingsID id, QString value)
{
    Setting_t s = this->getSetting(id);
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

#define GUI_SECTION "Rosalie's Mupen GUI"
#define CORE_SECTION GUI_SECTION " Core"
#define M64P_SECTION "Core"

Setting_t Settings::getSetting(SettingsID id)
{
    Setting_t setting = {"", "", "", "", false};

    switch (id)
    {
    default:
        break;
    case SettingsID::GUI_RomBrowserDirectory:
        setting = {GUI_SECTION, "ROM Browser Directory", "", "", false};
        break;
    case SettingsID::GUI_RomBrowserGeometry:
        setting = {GUI_SECTION, "ROM Browser Geometry", "", "", false};
        break;
    case SettingsID::GUI_SettingsDialogWidth:
        setting = {GUI_SECTION, "Settings Dialog Width", 0, "", false};
        break;
    case SettingsID::GUI_SettingsDialogHeight:
        setting = {GUI_SECTION, "Settings Dialog Height", 0, "", false};
        break;
    case SettingsID::GUI_AllowManualResizing:
        setting = {GUI_SECTION, "Allow Manual Resizing", false, "", false};
        break;
        /*
        case SettingsID::GUI_PauseEmulationOnFocusLoss:
            setting = {GUI_SECTION, "PauseEmulationOnFocusLoss", true, "", false};
            break;
        case SettingsID::GUI_ResumeEmulationOnFocus:
            setting = {GUI_SECTION, "ResumeEmulationOnFocus", true, "", false};
            break;
        */

    case SettingsID::Core_GFX_Plugin:
        setting = {CORE_SECTION, "GFX Plugin", "", "", false};
        break;
    case SettingsID::Core_AUDIO_Plugin:
        setting = {CORE_SECTION, "Audio Plugin", "", "", false};
        break;
    case SettingsID::Core_INPUT_Plugin:
        setting = {CORE_SECTION, "Input Plugin", "", "", false};
        break;
    case SettingsID::Core_RSP_Plugin:
        setting = {CORE_SECTION, "RSP Plugin", "", "", false};
        break;

    case SettingsID::Core_OverrideUserDirs:
        setting = {CORE_SECTION, "OverrideUserDirectories", true, "", false};
        break;
    case SettingsID::Core_UserDataDirOverride:
        setting = {CORE_SECTION, "UserDataDirectory", "Data", "", false};
        break;
    case SettingsID::Core_UserCacheDirOverride:
        setting = {CORE_SECTION, "UserCacheDirectory", "Cache", "", false};
        break;

    case SettingsID::Core_OverrideGameSpecificSettings:
        setting = {CORE_SECTION, "OverrideGameSpecificSettings", false, "", false};
        break;
    case SettingsID::Core_RandomizeInterrupt:
        setting = {M64P_SECTION, "RandomizeInterrupt", true, "", false};
        break;
    case SettingsID::Core_CPU_Emulator:
        setting = {M64P_SECTION, "R4300Emulator", 2, "", false};
        break;
    case SettingsID::Core_DisableExtraMem:
        setting = {M64P_SECTION, "DisableExtraMem", false, "", false};
        break;
    case SettingsID::Core_EnableDebugger:
        setting = {M64P_SECTION, "EnableDebugger", false, "", false};
        break;
    case SettingsID::Core_CountPerOp:
        setting = {M64P_SECTION, "CountPerOp", 0, "", false};
        break;
    case SettingsID::Core_SiDmaDuration:
        setting = {M64P_SECTION, "SiDmaDuration", -1, "", false};
        break;

    case SettingsID::Core_ScreenshotPath:
        setting = {M64P_SECTION, "ScreenshotPath", "Screenshots", "", true};
        break;
    case SettingsID::Core_SaveStatePath:
        setting = {M64P_SECTION, "SaveStatePath", "Save/State", "", true};
        break;
    case SettingsID::Core_SaveSRAMPath:
        setting = {M64P_SECTION, "SaveSRAMPath", "Save/Game", "", true};
        break;
    case SettingsID::Core_SharedDataPath:
        setting = {M64P_SECTION, "SharedDataPath", "Data", "", true};
        break;
    }

    return setting;
}
