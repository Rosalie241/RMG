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
#include "RomHeader.hpp"
#include "ConvertStringEncoding.hpp"
#include "Emulation.hpp"
#include "m64p/Api.hpp"
#include "Error.hpp"
#include "Rom.hpp"

//
// Exported Functions
//

bool CoreGetCurrentRomHeader(CoreRomHeader& header)
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
    header.Name        = CoreConvertStringEncoding((char*)m64p_header.Name, CoreStringEncoding::Shift_JIS);

    header.GameID.clear();
    header.GameID.push_back(char(ntohl(m64p_header.Manufacturer_ID)));
    header.GameID.push_back(char(m64p_header.Cartridge_ID % 256));
    header.GameID.push_back(char(m64p_header.Cartridge_ID / 256));
    header.GameID.push_back(char(header.CountryCode));

    if (header.GameID[0] == '\0')
    {
        header.GameID = "????";
    }
    
    switch (header.CountryCode)
    {
        case 65:
        // A
        header.Region = "Region-Free";
        break;

        case 66:
        // B
        header.Region = "Brazil";
        break;

        case 67:
        // C
        header.Region = "China";
        break;

        case 68:
        // D
        header.Region = "Germany";
        break;

        case 69:
        // E
        header.Region = "North America";
        break;

        case 70:
        // F
        header.Region = "France";
        break;

        case 71:
        // G
        header.Region = "Gateway 64 (NTSC)";
        break;

        case 72:
        // H
        header.Region = "Netherlands";
        break;

        case 73:
        // I
        header.Region = "Italy";
        break;

        case 74:
        // J
        header.Region = "Japan";
        break;

        case 75:
        // K
        header.Region = "Korea";
        break;

        case 76:
        // L
        header.Region = "Gateway 64 (PAL)";
        break;

        case 78:
        // N
        header.Region = "Canada";
        break;

        case 80:
        // P
        header.Region = "Europe/Australia";
        break;

        case 83:
        // S
        header.Region = "Spain";
        break;

        case 85:
        // U
        header.Region = "Australia";
        break;

        case 87:
        // W
        header.Region = "Scandanavia";
        break;

        case 88:
        // X
        header.Region = "Europe/Australia";
        break;

        case 89:
        // Y
        header.Region = "Europe";
        break;

        case 90:
        // Z
        header.Region = "Europe";
        break;

        default:
        header.Region = "Unknown";
        break;
    }

    return true;
}
