/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifdef _WIN32
#include <winsock2.h>
#else // Unix
#include <arpa/inet.h>
#endif // _WIN32

#define CORE_INTERNAL
#include "ConvertStringEncoding.hpp"
#include "RomHeader.hpp"
#include "m64p/Api.hpp"
#include "Library.hpp"
#include "Error.hpp"

//
// Local Functions
//

static std::string get_gameid_from_header(m64p_rom_header header)
{
    std::string gameID;

    if (header.Manufacturer_ID == 0)
    {
        return "????";
    }

    gameID.push_back(char(ntohl(header.Manufacturer_ID)));
    gameID.push_back(char(header.Cartridge_ID % 256));
    gameID.push_back(char(header.Cartridge_ID / 256));
    gameID.push_back(char(header.Country_code));

    return gameID;
}

static std::string get_region_from_countrycode(char countryCode)
{
    std::string region;

    switch (countryCode)
    {
        case 'A':
            region = "Region-Free";
            break;
        case 'B':
            region = "Brazil";
            break;
        case 'C':
            region = "China";
            break;
        case 'D':
            region = "Germany";
            break;
        case 'E':
            region = "North America";
            break;
        case 'F':
            region = "France";
            break;
        case 'G':
            region = "Gateway 64 (NTSC)";
            break;
        case 'H':
            region = "Netherlands";
            break;
        case 'I':
            region = "Italy";
            break;
        case 'J':
            region = "Japan";
            break;
        case 'K':
            region = "Korea";
            break;
        case 'L':
            region = "Gateway 64 (PAL)";
            break;
        case 'N':
            region = "Canada";
            break;
        case 'P':
        case 'X':
            region = "Europe/Australia";
            break;
        case 'S':
            region = "Spain";
            break;
        case 'U':
            region = "Australia";
            break;
        case 'W':
            region = "Scandanavia";
            break;
        case 'Y':
        case 'Z':
            region = "Europe";
            break;
        default:
            region = "Unknown";
            break;
    }

    return region;
}

static CoreSystemType get_systemtype_from_countrycode(uint8_t countryCode)
{
    CoreSystemType systemType;

    // taken from mupen64plus-core's rom.c
    // TODO: maybe update it upstream?
    switch (countryCode)
    {
        // PAL codes
        case 0x44:
        case 0x46:
        case 0x49:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x58:
        case 0x59:
            systemType = CoreSystemType::PAL;
            break;

        // NTSC codes
        case 0x37:
        case 0x41:
        case 0x45:
        case 0x4a:
        default: // Fallback for unknown codes
            systemType = CoreSystemType::NTSC;
            break;
    }

    return systemType;
}

//
// Exported Functions
//

CORE_EXPORT bool CoreGetCurrentRomHeader(CoreRomHeader& header)
{
    std::string error;
    m64p_error ret;
    m64p_rom_header m64p_header;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_ROM_GET_HEADER, sizeof(m64p_rom_settings), &m64p_header);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetCurrentRomHeader m64p::Core.DoCommand(M64CMD_ROM_GET_HEADER) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    header.CRC1        = ntohl(m64p_header.CRC1);
    header.CRC2        = ntohl(m64p_header.CRC2);
    header.CountryCode = m64p_header.Country_code;
    header.Name        = CoreConvertStringEncoding(std::string(reinterpret_cast<char*>(m64p_header.Name), 20), CoreStringEncoding::Shift_JIS);
    header.GameID      = get_gameid_from_header(m64p_header);
    header.Region      = get_region_from_countrycode(static_cast<char>(header.CountryCode));
    header.SystemType  = get_systemtype_from_countrycode(header.CountryCode);

    return true;
}
