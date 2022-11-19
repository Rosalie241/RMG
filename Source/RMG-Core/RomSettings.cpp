/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomSettings.hpp"
#include "Emulation.hpp"
#include "m64p/Api.hpp"
#include "Error.hpp"
#include "Rom.hpp"

#include "Settings/Settings.hpp"

//
// Local Variables
//

static CoreRomSettings l_DefaultRomSettings;
static bool            l_HasDefaultRomSettings = false;

//
// Exported Functions
//

bool CoreGetCurrentRomSettings(CoreRomSettings& settings)
{
    std::string       error;
    m64p_error        ret;
    m64p_rom_settings m64p_settings;

    ret = m64p::Core.DoCommand(M64CMD_ROM_GET_SETTINGS, sizeof(m64p_rom_settings), &m64p_settings);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetCurrentRomSettings m64p::Core.DoCommand(M64CMD_ROM_GET_SETTINGS) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    settings.GoodName = std::string(m64p_settings.goodname);
    settings.MD5 = std::string(m64p_settings.MD5);
    settings.SaveType = m64p_settings.savetype;
    settings.DisableExtraMem = m64p_settings.disableextramem;
    settings.CountPerOp = m64p_settings.countperop;
    settings.SiDMADuration = m64p_settings.sidmaduration;
    return true;
}

bool CoreStoreCurrentDefaultRomSettings(void)
{
    CoreRomSettings settings;

    if (!CoreGetCurrentRomSettings(settings))
    {
        return false;
    }

    l_DefaultRomSettings = settings;
    l_HasDefaultRomSettings = true;
    return true;
}

bool CoreClearCurrentDefaultRomSettings(void)
{
    l_HasDefaultRomSettings = false;
    return true;
}

bool CoreGetCurrentDefaultRomSettings(CoreRomSettings& settings)
{
    std::string error;

    if (!l_HasDefaultRomSettings)
    {
        error = "CoreGetCurrentDefaultRomSettings Failed: ";
        error += "cannot retreive default ROM settings when no defaults have been stored!";
        CoreSetError(error);
        return false;
    }

    settings = l_DefaultRomSettings;
    return true;
}

bool CoreApplyRomSettings(CoreRomSettings settings)
{
    std::string       error;
    m64p_error        ret;
    m64p_rom_settings m64p_settings;

    if (!CoreHasRomOpen())
    {
        error = "CoreApplyRomSettings Failed: ";
        error += "cannot apply rom settings when no ROM has been opened!";
        CoreSetError(error);
        return false;
    }

    // retrieve ROM settings
    ret = m64p::Core.DoCommand(M64CMD_ROM_GET_SETTINGS, sizeof(m64p_rom_settings), &m64p_settings);
    if (ret != M64ERR_SUCCESS)
    {
        CoreCloseRom();
        error = "CoreApplyRomSettings m64p::Core.DoCommand(M64CMD_ROM_GET_SETTINGS) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    // overide a few settings
    m64p_settings.savetype = settings.SaveType;
    m64p_settings.disableextramem = settings.DisableExtraMem;
    m64p_settings.countperop = settings.CountPerOp;
    m64p_settings.sidmaduration = settings.SiDMADuration;

    // apply ROM settings
    ret = m64p::Core.DoCommand(M64CMD_ROM_SET_SETTINGS, sizeof(m64p_rom_settings), &m64p_settings);
    if (ret != M64ERR_SUCCESS)
    {
        CoreCloseRom();
        error = "CoreApplyRomSettings m64p::Core.DoCommand(M64CMD_ROM_SET_SETTINGS) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreApplyRomSettingsOverlay(void)
{
    CoreRomSettings settings;

    if (!CoreGetCurrentDefaultRomSettings(settings))
    {
        return false;
    }

    // don't do anything when section doesn't exist
    if (!CoreSettingsSectionExists(settings.MD5))
    {
        return false;
    }

    // or when we don't override the settings
    if (!CoreSettingsGetBoolValue(SettingsID::Game_OverrideSettings, settings.MD5))
    {
        return false;
    }

    settings.SaveType = CoreSettingsGetIntValue(SettingsID::Game_SaveType, settings.MD5);
    settings.DisableExtraMem = CoreSettingsGetIntValue(SettingsID::Game_DisableExtraMem, settings.MD5);
    settings.CountPerOp = CoreSettingsGetIntValue(SettingsID::Game_CountPerOp, settings.MD5);
    settings.SiDMADuration = CoreSettingsGetIntValue(SettingsID::Game_SiDmaDuration, settings.MD5);

    return CoreApplyRomSettings(settings);
}
