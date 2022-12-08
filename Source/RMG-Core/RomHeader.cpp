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
    
    return true;
}
