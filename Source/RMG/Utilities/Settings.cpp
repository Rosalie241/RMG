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
#include "UserInterface/Widget/ColumnID.hpp"
#include "Utilities/SettingsID.hpp"
#include <QVariant>

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
    for (int i = 0; i < (int)SettingsID::Invalid; i++)
    {
        setting = this->getSetting((SettingsID)i);

        if (setting.Section.isEmpty())
            continue;
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
        case QVariant::Type::List: {
            QString str = "";
            for (QVariant &v : setting.Default.toList())
            {
                // TODO, support other QList<T>s aswell
                str += QString::number(v.toInt()) + ";";
            }
            g_MupenApi.Config.SetDefaultOption(setting.Section, setting.Key, str, setting.Help);
            break;
        }
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
    return this->getDefaultIntValue(this->getSetting(id));
}

bool Settings::GetDefaultBoolValue(SettingsID id)
{
    return this->getDefaultBoolValue(this->getSetting(id));
}

float Settings::GetDefaultFloatValue(SettingsID id)
{
    return this->getDefaultFloatValue(this->getSetting(id));
}

QString Settings::GetDefaultStringValue(SettingsID id)
{
    return this->getDefaultStringValue(this->getSetting(id));
}

QList<int> Settings::GetDefaultIntListValue(SettingsID id)
{
    return this->getDefaultIntListValue(this->getSetting(id));
}

int Settings::GetDefaultIntValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getDefaultIntValue(s);
}

bool Settings::GetDefaultBoolValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getDefaultBoolValue(s);
}

float Settings::GetDefaultFloatValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getDefaultFloatValue(s);
}

QString Settings::GetDefaultStringValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getDefaultStringValue(s);
}

QList<int> Settings::GetDefaultIntListValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getDefaultIntListValue(s);
}

int Settings::GetIntValue(SettingsID id)
{
    return this->getIntValue(this->getSetting(id));
}

bool Settings::GetBoolValue(SettingsID id)
{
    return this->getBoolValue(this->getSetting(id));
}

float Settings::GetFloatValue(SettingsID id)
{
    return this->getFloatValue(this->getSetting(id));
}

QString Settings::GetStringValue(SettingsID id)
{
    return this->getStringValue(this->getSetting(id));
}

QList<int> Settings::GetIntListValue(SettingsID id)
{
    return this->getIntListValue(this->getSetting(id));
}

int Settings::GetIntValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getIntValue(s);
}

bool Settings::GetBoolValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getBoolValue(s);
}

float Settings::GetFloatValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getFloatValue(s);
}

QString Settings::GetStringValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getStringValue(s);
}

QList<int> Settings::GetIntListValue(SettingsID id, QString section)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->getIntListValue(s);
}

bool Settings::SetValue(SettingsID id, int value)
{
    return this->setValue(this->getSetting(id), value);
}

bool Settings::SetValue(SettingsID id, bool value)
{
    return this->setValue(this->getSetting(id), value);
}

bool Settings::SetValue(SettingsID id, float value)
{
    return this->setValue(this->getSetting(id), value);
}

bool Settings::SetValue(SettingsID id, QString value)
{
    return this->setValue(this->getSetting(id), value);
}

bool Settings::SetValue(SettingsID id, QList<int> value)
{
    return this->setValue(this->getSetting(id), value);
}

bool Settings::SetValue(SettingsID id, QString section, int value)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->setValue(s, value);
}

bool Settings::SetValue(SettingsID id, QString section, bool value)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->setValue(s, value);
}

bool Settings::SetValue(SettingsID id, QString section, float value)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->setValue(s, value);
}

bool Settings::SetValue(SettingsID id, QString section, QString value)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->setValue(s, value);
}

bool Settings::SetValue(SettingsID id, QString section, QList<int> value)
{
    Setting_t s = this->getSetting(id);
    s.Section = section;
    return this->setValue(s, value);
}

#define GUI_SECTION "Rosalie's Mupen GUI"
#define CORE_SECTION GUI_SECTION " Core"
#define OVERLAY_SECTION CORE_SECTION " Overlay"
#define KEYBIND_SECTION GUI_SECTION " KeyBindings"
#define ROMBROWSER_SECTION GUI_SECTION " RomBrowser"
#define M64P_SECTION "Core"

Setting_t Settings::getSetting(SettingsID id)
{
    Setting_t setting = {"", "", "", "", false};

    switch (id)
    {
    default:
        break;
    case SettingsID::GUI_RomBrowserDirectory:
        setting = {GUI_SECTION, "RomBrowserDirectory", "", "", false};
        break;
    case SettingsID::GUI_RomBrowserGeometry:
        setting = {GUI_SECTION, "RomBrowserGeometry", "", "", false};
        break;
    case SettingsID::GUI_SettingsDialogWidth:
        setting = {GUI_SECTION, "SettingsDialogWidth", 0, "", false};
        break;
    case SettingsID::GUI_SettingsDialogHeight:
        setting = {GUI_SECTION, "SettingsDialogHeight", 0, "", false};
        break;
    case SettingsID::GUI_AllowManualResizing:
        setting = {GUI_SECTION, "AllowManualResizing", true, "", false};
        break;
    case SettingsID::GUI_HideCursorInEmulation:
        setting = {GUI_SECTION, "HideCursorInEmulation", false, "", false};
        break;
    case SettingsID::GUI_StatusbarMessageDuration:
        setting = {GUI_SECTION, "StatusbarMessageDuration", 3, "", false};
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
        setting = {CORE_SECTION, "GFX_Plugin", "", "", false};
        break;
    case SettingsID::Core_AUDIO_Plugin:
        setting = {CORE_SECTION, "AUDIO_Plugin", "", "", false};
        break;
    case SettingsID::Core_INPUT_Plugin:
        setting = {CORE_SECTION, "INPUT_Plugin", "", "", false};
        break;
    case SettingsID::Core_RSP_Plugin:
        setting = {CORE_SECTION, "RSP_Plugin", "", "", false};
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
        setting = {OVERLAY_SECTION, "RandomizeInterrupt", true, "", false};
        break;
    case SettingsID::Core_CPU_Emulator:
        setting = {OVERLAY_SECTION, "CPU_Emulator", 2, "", false};
        break;
    case SettingsID::Core_DisableExtraMem:
        setting = {OVERLAY_SECTION, "DisableExtraMem", false, "", false};
        break;
    case SettingsID::Core_EnableDebugger:
        setting = {OVERLAY_SECTION, "EnableDebugger", false, "", false};
        break;
    case SettingsID::Core_CountPerOp:
        setting = {OVERLAY_SECTION, "CountPerOp", 0, "", false};
        break;
    case SettingsID::Core_SiDmaDuration:
        setting = {OVERLAY_SECTION, "SiDmaDuration", -1, "", false};
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

    case SettingsID::Core_64DD_RomFile:
        setting = {CORE_SECTION, "64DD_RomFile", "", "", false};
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
        setting = {KEYBIND_SECTION, "OpenROM", "Ctrl+O", "", false};
        break;
    case SettingsID::KeyBinding_OpenCombo:
        setting = {KEYBIND_SECTION, "OpenCombo", "Ctrl+Shift+O", "", false};
        break;
    case SettingsID::KeyBinding_StartEmulation:
        setting = {KEYBIND_SECTION, "StartEmulation", "F11", "", false};
        break;
    case SettingsID::KeyBinding_EndEmulation:
        setting = {KEYBIND_SECTION, "EndEmulation", "F12", "", false};
        break;
    case SettingsID::KeyBinding_RefreshROMList:
        setting = {KEYBIND_SECTION, "RefreshROMList", "F5", "", false};
        break;
    case SettingsID::KeyBinding_Exit:
        setting = {KEYBIND_SECTION, "Exit", "Alt+F4", "", false};
        break;
    case SettingsID::KeyBinding_SoftReset:
        setting = {KEYBIND_SECTION, "SoftReset", "F1", "", false};
        break;
    case SettingsID::KeyBinding_HardReset:
        setting = {KEYBIND_SECTION, "HardReset", "Shift+F1", "", false};
        break;
    case SettingsID::KeyBinding_Resume:
        setting = {KEYBIND_SECTION, "Resume", "F2", "", false};
        break;
    case SettingsID::KeyBinding_GenerateBitmap:
        setting = {KEYBIND_SECTION, "GenerateBitmap", "F3", "", false};
        break;
    case SettingsID::KeyBinding_LimitFPS:
        setting = {KEYBIND_SECTION, "LimitFPS", "F4", "", false};
        break;
    case SettingsID::KeyBinding_SwapDisk:
        setting = {KEYBIND_SECTION, "SwapDisk", "Ctrl+D", "", false};
        break;
    case SettingsID::KeyBinding_SaveState:
        setting = {KEYBIND_SECTION, "SaveState", "F5", "", false};
        break;
    case SettingsID::KeyBinding_SaveAs:
        setting = {KEYBIND_SECTION, "SaveAs", "Ctrl+S", "", false};
        break;
    case SettingsID::KeyBinding_LoadState:
        setting = {KEYBIND_SECTION, "LoadState", "F7", "", false};
        break;
    case SettingsID::KeyBinding_Load:
        setting = {KEYBIND_SECTION, "Load", "Ctrl+L", "", false};
        break;
    case SettingsID::KeyBinding_Cheats:
        setting = {KEYBIND_SECTION, "Cheats", "Ctrl+C", "", false};
        break;
    case SettingsID::KeyBinding_GSButton:
        setting = {KEYBIND_SECTION, "GSButton", "F9", "", false};
        break;
    case SettingsID::KeyBinding_Fullscreen:
        setting = {KEYBIND_SECTION, "Fullscreen", "Alt+Return", "", false};
        break;
    case SettingsID::KeyBinding_Settings:
        setting = {KEYBIND_SECTION, "Settings", "Ctrl+T", "", false};
        break;

    case SettingsID::RomBrowser_Recursive:
        setting = {ROMBROWSER_SECTION, "Recursive", true, "", false};
        break;
    case SettingsID::RomBrowser_MaxItems:
        setting = {ROMBROWSER_SECTION, "MaxItems", 50, "", false};
        break;
    case SettingsID::RomBrowser_Columns:
        setting = {ROMBROWSER_SECTION, "Columns", QVariant({(int)ColumnID::GoodName, (int)ColumnID::InternalName, (int)ColumnID::MD5}), "", false};
        break;
    case SettingsID::RomBrowser_ColumnSizes:
        setting = {ROMBROWSER_SECTION, "ColumnSizes", QVariant({(int)ColumnID::GoodName, 250, (int)ColumnID::InternalName, 100, (int)ColumnID::MD5, 100}), "", false};
        break;
    }

    return setting;
}

int Settings::getDefaultIntValue(Setting_t s)
{
    return s.Default.toInt();
}

bool Settings::getDefaultBoolValue(Setting_t s)
{
    return s.Default.toBool();
}

float Settings::getDefaultFloatValue(Setting_t s)
{
    return s.Default.toFloat();
}

QString Settings::getDefaultStringValue(Setting_t s)
{
    return s.Default.toString();
}

QList<int> Settings::getDefaultIntListValue(Setting_t s)
{
    QList<int> value;

    for (const QVariant& var : s.Default.toList())
    {
        value.append(var.toInt());
    }

    return value;
}

int Settings::getIntValue(Setting_t s)
{
    int value = s.Default.toInt();
    if (!s.Section.isEmpty() && g_MupenApi.Config.SectionExists(s.Section))
        g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

bool Settings::getBoolValue(Setting_t s)
{
    bool value = s.Default.toBool();
    if (!s.Section.isEmpty() && g_MupenApi.Config.SectionExists(s.Section))
        g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

float Settings::getFloatValue(Setting_t s)
{
    float value = s.Default.toFloat();
    if (!s.Section.isEmpty() && g_MupenApi.Config.SectionExists(s.Section))
        g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

QString Settings::getStringValue(Setting_t s)
{
    QString value = s.Default.toString();
    if (!s.Section.isEmpty() && g_MupenApi.Config.SectionExists(s.Section))
        g_MupenApi.Config.GetOption(s.Section, s.Key, &value);
    return value;
}

QList<int> Settings::getIntListValue(Setting_t s)
{
    QList<int> value;
    if (!s.Section.isEmpty() && g_MupenApi.Config.SectionExists(s.Section))
    {
        QString strValue = this->getStringValue(s);
        for (const QString& str : strValue.split(";"))
        {
            if (str.isEmpty())
            {
                continue;
            }
            value.append(str.toInt());
        }
    }
    if (value.isEmpty())
    {
        value = this->getDefaultIntListValue(s);
    }
    return value;
}

bool Settings::setValue(Setting_t s, int value)
{
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

bool Settings::setValue(Setting_t s, bool value)
{
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

bool Settings::setValue(Setting_t s, float value)
{
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

bool Settings::setValue(Setting_t s, QString value)
{
    bool ret = false;
    ret = g_MupenApi.Config.SetOption(s.Section, s.Key, value);
    return ret;
}

bool Settings::setValue(Setting_t s, QList<int> value)
{
    QString str;
    for (const int& val : value)
    {
        str += QString::number(val) + ";";
    }

    return this->setValue(s, str);
}
