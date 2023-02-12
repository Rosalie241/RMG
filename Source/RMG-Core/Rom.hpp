/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_ROM_HPP
#define CORE_ROM_HPP

#include <filesystem>

enum class CoreRomType
{
    Cartridge = 0,
    Disk      = 1
};

// opens the given file as ROM
bool CoreOpenRom(std::filesystem::path file);

// returns whether core has a ROM opened
bool CoreHasRomOpen(void);

// attempts to retrieve the ROM type
bool CoreGetRomType(CoreRomType& type);

// closes ROM
bool CoreCloseRom(void);

#endif // CORE_ROM_HPP