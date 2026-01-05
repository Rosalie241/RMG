/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "MediaLoader.hpp"
#include "RomSettings.hpp"
#include "Emulation.hpp"
#include "RomHeader.hpp"
#include "Settings.hpp"
#include "Library.hpp"
#include "Netplay.hpp"
#include "Kaillera.hpp"
#include "Plugins.hpp"
#include "Cheats.hpp"
#include "Error.hpp"
#include "File.hpp"
#include "Rom.hpp"

#include "m64p/Api.hpp"

// Windows/POSIX dynamic loading
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Forward declarations for PIF structures
extern "C" {
    struct pif;
    struct pif_channel {
        void* jbd;
        const void* ijbd;
        uint8_t* tx;
        uint8_t* tx_buf;
        uint8_t* rx;
        uint8_t* rx_buf;
    };
    struct pif {
        uint8_t* base;
        uint8_t* ram;
        struct pif_channel channels[6];  // PIF_CHANNELS_COUNT = 6
    };

    // Joybus command constants
    enum {
        JCMD_STATUS = 0x00,
        JCMD_CONTROLLER_READ = 0x01,
        JCMD_PAK_READ = 0x02,
        JCMD_PAK_WRITE = 0x03,
        JCMD_EEPROM_READ = 0x04,
        JCMD_EEPROM_WRITE = 0x05,
        JCMD_RESET = 0xff
    };

    typedef void (*pif_sync_callback_t)(struct pif*);
}

//
// Local Variables
//

// Frame counter for Kaillera sync (updated via frame callback)
static int s_CurrentFrame = 0;

// Frame callback function
static void FrameCallback(unsigned int frameIndex)
{
    s_CurrentFrame = frameIndex;
}

// Kaillera PIF sync callback (called from mupen64plus-core after netplay sync)
static void KailleraPifSyncCallback(struct pif* pif)
{
#ifdef NETPLAY
    if (!CoreHasInitKaillera()) {
        return;
    }

    int player_num = CoreGetKailleraPlayerNumber();
    int num_players = CoreGetKailleraNumPlayers();

    if (player_num < 1 || player_num > 4) {
        return; // Invalid player number
    }

    // Extract local input from the FIRST active controller (channel 0)
    // Each player uses their local controller, regardless of player number
    uint32_t local_input = 0;

    if (pif->channels[0].tx &&
        pif->channels[0].tx_buf[0] == JCMD_CONTROLLER_READ &&
        pif->channels[0].rx_buf != NULL) {
        // Read 4-byte controller response from local controller
        // N64 controller format: [buttons_hi][buttons_lo][x_axis][y_axis]
        uint8_t* rx = pif->channels[0].rx_buf;
        local_input = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3];
    }

    // Synchronize via Kaillera (send our input, get all inputs back)
    uint32_t sync_buffer[4] = {0};
    sync_buffer[0] = local_input;

    int ret = CoreModifyKailleraPlayValues(sync_buffer, sizeof(uint32_t));

    // Write back synchronized inputs to PIF RAM for all netplay players
    // This makes remote players appear as physically connected controllers
    if (ret > 0) {
        int num_received = ret / sizeof(uint32_t);

        for (int i = 0; i < num_received && i < 4; i++) {
            if (pif->channels[i].tx && pif->channels[i].rx != NULL) {
                // Always clear error bits to show controller as connected
                *pif->channels[i].rx &= ~0xC0;

                uint8_t cmd = pif->channels[i].tx_buf[0];

                if (cmd == JCMD_STATUS || cmd == JCMD_RESET) {
                    // Controller detection - force standard controller type response
                    if (pif->channels[i].rx_buf != NULL) {
                        uint16_t type = 0x0500; // JDT_JOY_ABS_COUNTERS | JDT_JOY_PORT
                        pif->channels[i].rx_buf[0] = (uint8_t)(type >> 0);
                        pif->channels[i].rx_buf[1] = (uint8_t)(type >> 8);
                        pif->channels[i].rx_buf[2] = 0; // No pak status
                    }
                }
                else if (cmd == JCMD_CONTROLLER_READ) {
                    // Write synced controller input
                    if (pif->channels[i].rx_buf != NULL) {
                        uint8_t* rx = pif->channels[i].rx_buf;
                        rx[0] = (sync_buffer[i] >> 24) & 0xFF;
                        rx[1] = (sync_buffer[i] >> 16) & 0xFF;
                        rx[2] = (sync_buffer[i] >> 8) & 0xFF;
                        rx[3] = sync_buffer[i] & 0xFF;
                    }
                }
                else if (cmd == JCMD_PAK_READ && pif->channels[i].rx_buf != NULL) {
                    // No controller pak present
                    pif->channels[i].rx_buf[32] = 255;
                }
                else if (cmd == JCMD_PAK_WRITE && pif->channels[i].rx_buf != NULL) {
                    // No controller pak present
                    pif->channels[i].rx_buf[0] = 255;
                }
            }
        }
    }
#endif // NETPLAY
}

//
// Local Functions
//

static bool get_emulation_state(m64p_emu_state& state)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, &state);
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
    CoreSettingsSetValue(SettingsID::Core_GbCameraVideoCaptureBackend1, CoreSettingsGetStringValue(SettingsID::CoreOverlay_GbCameraVideoCaptureBackend1));
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

static void apply_pif_rom_settings(void)
{
    CoreRomHeader romHeader;
    std::string error;
    m64p_error ret;
    int cpuEmulator;
    bool usePifROM;

    // when we fail to retrieve the rom settings, return
    if (!CoreGetCurrentRomHeader(romHeader))
    {
        return;
    }

    // when we're using the dynarec, return
    cpuEmulator = CoreSettingsGetIntValue(SettingsID::Core_CPU_Emulator);
    if (cpuEmulator >= 2)
    {
        return;
    }

    usePifROM = CoreSettingsGetBoolValue(SettingsID::Core_PIF_Use);
    if (!usePifROM)
    {
        return;
    }

    const SettingsID settingsIds[] =
    {
        SettingsID::Core_PIF_NTSC,
        SettingsID::Core_PIF_PAL,
    };

    std::string rom = CoreSettingsGetStringValue(settingsIds[static_cast<int>(romHeader.SystemType)]);
    if (!std::filesystem::is_regular_file(rom))
    {
        return;
    }

    std::vector<char> buffer;
    if (!CoreReadFile(rom, buffer))
    {
        return;
    }

    ret = m64p::Core.DoCommand(M64CMD_PIF_OPEN, buffer.size(), buffer.data());
    if (ret != M64ERR_SUCCESS)
    {
        error = "open_pif_rom m64p::Core.DoCommand(M64CMD_PIF_OPEN) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }
}

//
// Exported Functions
//

CORE_EXPORT bool CoreStartEmulation(std::filesystem::path n64rom, std::filesystem::path n64ddrom, 
    std::string address, int port, int player)
{
    std::string error;
    m64p_error  m64p_ret;
    bool        netplay_ret = false;
    CoreRomType type;
    bool        netplay = !address.empty();

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

    if (netplay)
    { // netplay cheats
        if (!CoreApplyNetplayCheats())
        {
            CoreDetachPlugins();
            CoreApplyPluginSettings();
            CoreCloseRom();
            return false;
        }
    }
    else
    { // local cheats
        if (!CoreApplyCheats())
        {
            CoreDetachPlugins();
            CoreApplyPluginSettings();
            CoreCloseRom();
            return false;
        }
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

    // apply pif rom settings
    apply_pif_rom_settings();

#ifdef NETPLAY
    // Kaillera connection happens BEFORE emulation via kailleraSelectServerDialog
    // Just verify it's initialized if netplay was requested
    if (netplay)
    {
        // Check if address is "KAILLERA" marker (set by UI when using Kaillera)
        if (address == "KAILLERA")
        {
            if (!CoreHasInitKaillera())
            {
                CoreSetError("CoreStartEmulation: Kaillera not initialized");
                m64p_ret = M64ERR_SYSTEM_FAIL;
                netplay_ret = false;
            }
            else
            {
                // Store player number for input plugin to use
                CoreSetKailleraPlayerNumber(player);
                netplay_ret = true;
            }
        }
        else
        {
            // Legacy netplay (Mupen64Plus built-in)
            netplay_ret = CoreInitNetplay(address, port, player);
            if (!netplay_ret)
            {
                m64p_ret = M64ERR_SYSTEM_FAIL;
            }
        }
    }
#endif // NETPLAY

    // only start emulation when initializing netplay
    // is successful or if there's no netplay requested
    if (!netplay || netplay_ret)
    {
        // Register frame callback for frame counter (used by Kaillera)
        s_CurrentFrame = 0;
        m64p::Core.DoCommand(M64CMD_SET_FRAME_CALLBACK, 0, (void*)FrameCallback);

#ifdef NETPLAY
        // Register Kaillera PIF sync callback (works with any input plugin)
        // Get function pointer dynamically since mupen64plus is loaded at runtime
        typedef void (*set_pif_sync_callback_t)(pif_sync_callback_t);
        void* coreHandle = m64p::Core.GetHandle();
        if (coreHandle)
        {
#ifdef _WIN32
            set_pif_sync_callback_t set_callback =
                (set_pif_sync_callback_t)GetProcAddress((HMODULE)coreHandle, "set_pif_sync_callback");
#else
            set_pif_sync_callback_t set_callback =
                (set_pif_sync_callback_t)dlsym(coreHandle, "set_pif_sync_callback");
#endif
            if (set_callback)
            {
                set_callback(KailleraPifSyncCallback);
            }
        }
#endif

        m64p_ret = m64p::Core.DoCommand(M64CMD_EXECUTE, 0, nullptr);
        if (m64p_ret != M64ERR_SUCCESS)
        {
            error = "CoreStartEmulation m64p::Core.DoCommand(M64CMD_EXECUTE) Failed: ";
            error += m64p::Core.ErrorMessage(m64p_ret);
        }
    }

#ifdef NETPLAY
    if (netplay && netplay_ret)
    {
        // Check if we used Kaillera or legacy netplay
        if (address == "KAILLERA")
        {
            CoreEndKailleraGame();
            CoreShutdownKaillera();
        }
        else
        {
            CoreShutdownNetplay();
        }
    }
#endif // NETPLAY

    CoreClearCheats();
    CoreDetachPlugins();
    CoreCloseRom();

    // restore plugin settings
    CoreApplyPluginSettings();

    // reset media loader state
    CoreResetMediaLoader();

    if (!netplay || netplay_ret)
    {
        // we need to set the emulation error last,
        // to prevent the other functions from
        // overriding the emulation error
        CoreSetError(error);
    }

    return m64p_ret == M64ERR_SUCCESS;
}

CORE_EXPORT bool CoreStopEmulation(void)
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

#ifdef NETPLAY
    // Clear Kaillera player number when stopping
    CoreSetKailleraPlayerNumber(0);
#endif

    return ret == M64ERR_SUCCESS;
}

CORE_EXPORT bool CorePauseEmulation(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    if (CoreHasInitNetplay() || CoreHasInitKaillera())
    {
        return false;
    }

    if (!CoreIsEmulationRunning())
    {
        error = "CorePauseEmulation Failed: ";
        error += "cannot pause emulation when emulation isn't running!";
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

CORE_EXPORT bool CoreResumeEmulation(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    if (CoreHasInitNetplay() || CoreHasInitKaillera())
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

CORE_EXPORT bool CoreResetEmulation(bool hard)
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

CORE_EXPORT bool CoreIsEmulationRunning(void)
{
    m64p_emu_state state = M64EMU_STOPPED;
    return get_emulation_state(state) && state == M64EMU_RUNNING;
}

CORE_EXPORT bool CoreIsEmulationPaused(void)
{
    m64p_emu_state state = M64EMU_STOPPED;
    return get_emulation_state(state) && state == M64EMU_PAUSED;
}

CORE_EXPORT int CoreGetCurrentFrameCount(void)
{
    // Return frame counter updated via frame callback
    return s_CurrentFrame;
}
