/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Settings/Settings.hpp"
#include "MediaLoader.hpp"
#include "RomSettings.hpp"
#include "Emulation.hpp"
#include "m64p/Api.hpp"
#include "Plugins.hpp"
#include "Cheats.hpp"
#include "Error.hpp"
#include "Rom.hpp"
#ifdef DISCORD_RPC
#include "DiscordRpc.hpp"
#endif // DISCORD_RPC

//
// Local Functions
//

static bool get_emulation_state(m64p_emu_state* state)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, state);
    if (ret != M64ERR_SUCCESS)
    {
        error = "get_emulation_state m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static void apply_coresettings_overlay(void)
{
    CoreSettingsSetValue(SettingsID::Core_RandomizeInterrupt, CoreSettingsGetBoolValue(SettingsID::CoreOverlay_RandomizeInterrupt));
    CoreSettingsSetValue(SettingsID::Core_CPU_Emulator, CoreSettingsGetIntValue(SettingsID::CoreOverlay_CPU_Emulator));
    CoreSettingsSetValue(SettingsID::Core_DisableExtraMem, CoreSettingsGetBoolValue(SettingsID::CoreOverlay_DisableExtraMem));
    CoreSettingsSetValue(SettingsID::Core_EnableDebugger, CoreSettingsGetBoolValue(SettingsID::CoreOverlay_EnableDebugger));
    CoreSettingsSetValue(SettingsID::Core_CountPerOp, CoreSettingsGetIntValue(SettingsID::CoreOverlay_CountPerOp));
    CoreSettingsSetValue(SettingsID::Core_CountPerOpDenomPot, CoreSettingsGetIntValue(SettingsID::CoreOverlay_CountPerOpDenomPot));
    CoreSettingsSetValue(SettingsID::Core_SiDmaDuration, CoreSettingsGetIntValue(SettingsID::CoreOverlay_SiDmaDuration));
    CoreSettingsSetValue(SettingsID::Core_SaveFileNameFormat, CoreSettingsGetIntValue(SettingsID::CoreOverLay_SaveFileNameFormat));
}

static void apply_game_coresettings_overlay(void)
{
    std::string section;
    CoreRomSettings romSettings;
    bool overrideCoreSettings;

    // when we fail to retrieve the rom settings, return
    if (!CoreGetCurrentDefaultRomSettings(romSettings))
    {
        return;
    }

    section = romSettings.MD5;

    // when we don't need to override the core settings, return
    overrideCoreSettings = CoreSettingsGetBoolValue(SettingsID::Game_OverrideCoreSettings, section);
    if (!overrideCoreSettings)
    {
        return;
    }

    // apply settings overlay
    CoreSettingsSetValue(SettingsID::Core_RandomizeInterrupt, CoreSettingsGetBoolValue(SettingsID::Game_RandomizeInterrupt, section));
    CoreSettingsSetValue(SettingsID::Core_CPU_Emulator, CoreSettingsGetIntValue(SettingsID::Game_CPU_Emulator, section));
    CoreSettingsSetValue(SettingsID::Core_CountPerOpDenomPot, CoreSettingsGetIntValue(SettingsID::Game_CountPerOpDenomPot, section));
}

//
// Exported Functions
//

bool CoreStartEmulation(std::filesystem::path n64rom, std::filesystem::path n64ddrom)
{
    std::string error;
    m64p_error  ret;
    CoreRomType type;

    if (!CoreOpenRom(n64rom))
    {
        return false;
    }

    if (!CoreApplyRomPluginSettings())
    {
        CoreApplyPluginSettings();
        CoreCloseRom();
        return false;
    }

    if (!CoreArePluginsReady())
    {
        CoreApplyPluginSettings();
        CoreCloseRom();
        return false;
    }

    if (!CoreAttachPlugins())
    {
        CoreApplyPluginSettings();
        CoreCloseRom();
        return false;
    }

    if (!CoreApplyCheats())
    {
        CoreDetachPlugins();
        CoreApplyPluginSettings();
        CoreCloseRom();
        return false;
    }

    if (!CoreGetRomType(type))
    {
        CoreClearCheats();
        CoreDetachPlugins();
        CoreApplyPluginSettings();
        CoreCloseRom();
        return false;
    }

    // set disk file in media loader when ROM is a cartridge
    if (type == CoreRomType::Cartridge)
    {
        CoreMediaLoaderSetDiskFile(n64ddrom);
    }

    // apply core settings overlay
    apply_coresettings_overlay();

    // apply game core settings overrides
    apply_game_coresettings_overlay();

#ifdef DISCORD_RPC
    CoreDiscordRpcUpdate(true);
#endif // DISCORD_RPC

    ret = m64p::Core.DoCommand(M64CMD_EXECUTE, 0, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreStartEmulation m64p::Core.DoCommand(M64CMD_EXECUTE) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
    }

    CoreClearCheats();
    CoreDetachPlugins();
    CoreCloseRom();

    // restore plugin settings
    CoreApplyPluginSettings();

    // reset media loader state
    CoreResetMediaLoader();

#ifdef DISCORD_RPC
    CoreDiscordRpcUpdate(false);
#endif // DISCORD_RPC

    // we need to set the emulation error last,
    // to prevent the other functions from
    // overriding the emulation error
    CoreSetError(error);

    return ret == M64ERR_SUCCESS;
}

bool CoreStopEmulation(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_STOP, 0, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreStopEmulation m64p::Core.DoCommand(M64CMD_STOP) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    return ret == M64ERR_SUCCESS;
}

bool CorePauseEmulation(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    if (!CoreIsEmulationRunning())
    {
        error = "CorePauseEmulation Failed: ";
        error += "cannot pause emulation when emulation isn't running!";\
        CoreSetError(error);
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_PAUSE, 0, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CorePauseEmulation m64p::Core.DoCommand(M64CMD_PAUSE) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreResumeEmulation(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    if (!CoreIsEmulationPaused())
    {
        error = "CoreIsEmulationPaused Failed: ";
        error += "cannot resume emulation when emulation isn't paused!";
        CoreSetError(error);
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_RESUME, 0, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreResumeEmulation m64p::Core.DoCommand(M64CMD_RESUME) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreResetEmulation(bool hard)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    if (CoreIsEmulationPaused())
    {
        error = "CoreResetEmulation Failed: ";
        error += "cannot reset emulation when paused!";
        CoreSetError(error);
        return false;
    }

    if (!CoreIsEmulationRunning())
    {
        error = "CoreResetEmulation Failed: ";
        error += "cannot reset emulation when emulation isn't running!";
        CoreSetError(error);
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_RESET, hard, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreResetEmulation m64p::Core.DoCommand(M64CMD_RESET) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreIsEmulationRunning(void)
{
    m64p_emu_state state = M64EMU_STOPPED;
    return get_emulation_state(&state) && state == M64EMU_RUNNING;
}

bool CoreIsEmulationPaused(void)
{
    m64p_emu_state state = M64EMU_STOPPED;
    return get_emulation_state(&state) && state == M64EMU_PAUSED;
}
