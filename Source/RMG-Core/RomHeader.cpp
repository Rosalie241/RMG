/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomHeader.hpp"
#include "Emulation.hpp"
#include "m64p/Api.hpp"
#include "Error.hpp"
#include "Rom.hpp"

#ifdef _WIN32
#include <winsock.h>
#else // Unix
#include <arpa/inet.h>
#endif // _WIN32

//
// Exported Functions
//

bool CoreGetCurrentRomHeader(CoreRomHeader& header)
{
    std::string error;
    m64p_error ret;
    m64p_rom_header m64p_header;

    ret = m64p::Core.DoCommand(M64CMD_ROM_GET_HEADER, sizeof(m64p_rom_settings), &m64p_header);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetCurrentRomHeader m64p::Core.DoCommand(M64CMD_ROM_GET_HEADER) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    header.CRC1 = ntohl(m64p_header.CRC1);
    header.CRC2 = ntohl(m64p_header.CRC2);
    header.Name = std::string((char*)m64p_header.Name);
    return true;
}
