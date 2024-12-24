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
#include "MediaLoader.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

#include <cstdint>
#include <string.h>

//
// Local Variables
//

static std::filesystem::path l_DdDiskFile;
static std::filesystem::path l_DdRomFile;

//
// Local Functions
//

static char* medialoader_get_dd_disk(void*)
{
    if (l_DdDiskFile.empty())
    {
        return nullptr;
    }

    return strdup(l_DdDiskFile.string().c_str());
}

static void medialoader_set_dd_rom_region(void*, uint8_t region)
{
    switch (region)
    {
        case DDREGION_JAPAN:
            l_DdRomFile = CoreSettingsGetStringValue(SettingsID::Core_64DD_JapaneseIPL);
            break;
        case DDREGION_US:
            l_DdRomFile = CoreSettingsGetStringValue(SettingsID::Core_64DD_AmericanIPL);
            break;
        case DDREGION_DEV:
            l_DdRomFile = CoreSettingsGetStringValue(SettingsID::Core_64DD_DevelopmentIPL);
            break;
        case DDREGION_UNKNOWN:
            l_DdRomFile.clear();
            break;
    }
}

static char* medialoader_get_dd_rom(void*)
{
    if (l_DdRomFile.empty())
    {
        return nullptr;
    }

    return strdup(l_DdRomFile.string().c_str());
}

static char* medialoader_get_gb_cart_ram(void*, int index)
{
    std::filesystem::path gameBoyRom;
    SettingsID settingIds[] =
    {
        SettingsID::Core_Gameboy_P1_Save,
        SettingsID::Core_Gameboy_P2_Save,
        SettingsID::Core_Gameboy_P3_Save,
        SettingsID::Core_Gameboy_P4_Save,
    };

    gameBoyRom = CoreSettingsGetStringValue(settingIds[index]);
    if (gameBoyRom.empty())
    {
        return nullptr;
    }

    return strdup(gameBoyRom.string().c_str());
}

static char* mediaLoader_get_gb_cart_rom(void*, int index)
{
    std::filesystem::path gameBoySave;
    SettingsID settingIds[] =
    {
        SettingsID::Core_Gameboy_P1_Rom,
        SettingsID::Core_Gameboy_P2_Rom,
        SettingsID::Core_Gameboy_P3_Rom,
        SettingsID::Core_Gameboy_P4_Rom,
    };

    gameBoySave = CoreSettingsGetStringValue(settingIds[index]);
    if (gameBoySave.empty())
    {
        return nullptr;
    }

    return strdup(gameBoySave.string().c_str());
}

//
// Exported Functions
//

bool CoreSetupMediaLoader(void)
{
    std::string error;
    m64p_error  ret;

    m64p_media_loader mediaLoader;
    mediaLoader.cb_data           = nullptr;
    mediaLoader.get_dd_disk       = medialoader_get_dd_disk;
    mediaLoader.set_dd_rom_region = medialoader_set_dd_rom_region;
    mediaLoader.get_dd_rom        = medialoader_get_dd_rom;
    mediaLoader.get_gb_cart_ram   = medialoader_get_gb_cart_ram;
    mediaLoader.get_gb_cart_rom   = mediaLoader_get_gb_cart_rom;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_SET_MEDIA_LOADER, sizeof(mediaLoader), &mediaLoader);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetupMediaLoader m64p::Core.DoCommand(M64CMD_SET_MEDIA_LOADER) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }
 
    return ret == M64ERR_SUCCESS;
}

void CoreResetMediaLoader(void)
{
    l_DdRomFile = "";
}

void CoreMediaLoaderSetDiskFile(std::filesystem::path disk)
{
    l_DdDiskFile = disk;
}
