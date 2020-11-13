/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SETTINGSTYPES_HPP
#define SETTINGSTYPES_HPP

#include "Config.hpp"

#include <QString>
#include <QVariant>

typedef struct
{
    QString Section;
    QString Key;
    QVariant Default;
    QString Help;
} Setting_t;

enum SettingsID
{
    // GUI Settings
    GUI_RomBrowserDirectory = 0,
    GUI_RomBrowserGeometry,
    GUI_SettingsDialogWidth,
    GUI_SettingsDialogHeight,

    // Core Plugin Settings
    Core_GFX_Plugin,
    Core_AUDIO_Plugin,
    Core_INPUT_Plugin,
    Core_RSP_Plugin,

    // Core User Directory Settings
    Core_OverrideUserDirs,
    Core_UserDataDirOverride,
    Core_UserCacheDirOverride,
    
    // (mupen64plus) Core Settings
    Core_RandomizeInterrupt,
    Core_CPU_Emulator,
    Core_DisableExtraMem,
    Core_EnableDebugger,
    Core_CountPerOp,
    Core_SiDmaDuration,

    // (mupen64plus) Core Directory Settings
    Core_ScreenshotPath,
    Core_SaveStatePath,
    Core_SaveSRAMPath,
    Core_SharedDataPath
};

#endif // SETTINGSTYPES_HPP