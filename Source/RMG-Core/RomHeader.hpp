/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_ROMHEADER_HPP
#define CORE_ROMHEADER_HPP

#include <cstdint>
#include <string>

struct CoreRomHeader
{
    uint32_t    CRC1;
    uint32_t    CRC2;
    uint32_t    CountryCode;
    std::string Name;
    std::string GameID;
    std::string Region;
};

// retrieves the currently opened ROM header
bool CoreGetCurrentRomHeader(CoreRomHeader& header);

#endif // CORE_ROMHEADER_HPP