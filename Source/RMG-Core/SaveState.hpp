/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_SAVESTATE_HPP
#define CORE_SAVESTATE_HPP

#include "RomHeader.hpp"
#include "RomSettings.hpp"

#include <filesystem>

enum class CoreSaveStateType
{
	Mupen64Plus = 1,
	Project64   = 2
};

// sets save state slot
bool CoreSetSaveStateSlot(int slot);

// retrieves current save state slot,
// returns -1 on error
int  CoreGetSaveStateSlot(void);

// attempts to increase the save state slot
bool CoreIncreaseSaveStateSlot(void);

// attempts to decrease the save state slot
bool CoreDecreaseSaveStateSlot(void);

// retrieves the file path for
// the save state in the given slot
// for the currently opened ROM
bool CoreGetSaveStatePath(int slot, std::filesystem::path& path);

// retrieves the file path for
// the save state in the given slot
bool CoreGetSaveStatePath(CoreRomHeader header, CoreRomSettings settings, int slot, std::filesystem::path& path);


// saves state
bool CoreSaveState(void);

// saves state to file
bool CoreSaveState(std::filesystem::path file, CoreSaveStateType type = CoreSaveStateType::Mupen64Plus);

// loads saved state
bool CoreLoadSaveState(void);

// loads saved state from file
bool CoreLoadSaveState(std::filesystem::path file);

#endif // CORE_SAVESTATE_HPP