/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_CACHEDROMHEADERANDSETTINGS_HPP
#define CORE_CACHEDROMHEADERANDSETTINGS_HPP

#include <filesystem>

#include "Rom.hpp"
#include "RomHeader.hpp"
#include "RomSettings.hpp"

#ifdef CORE_INTERNAL
// attempts to read rom header & settings cache
void CoreReadRomHeaderAndSettingsCache(void);

// returns whether saving the rom header & settings cache
// succeeds
bool CoreSaveRomHeaderAndSettingsCache(void);
#endif // CORE_INTERNAL

// returns whether retrieving the rom header & settings
// for given filename succeeds, it also attempts to add
// an entry if there's no cached entry found
bool CoreGetCachedRomHeaderAndSettings(std::filesystem::path file, CoreRomType* type, CoreRomHeader* header, CoreRomSettings* defaultSettings, CoreRomSettings* settings);

// returns whether adding cached rom header & settings
// for given filename succeeds
bool CoreAddCachedRomHeaderAndSettings(std::filesystem::path file, CoreRomType type, CoreRomHeader header, CoreRomSettings defaultSettings, CoreRomSettings settings);

// returns whether updating the cached rom header & settings succeeds
bool CoreUpdateCachedRomHeaderAndSettings(std::filesystem::path file, CoreRomType type, CoreRomHeader header, CoreRomSettings defaultSettings, CoreRomSettings settings);

#ifdef CORE_INTERNAL
// returns whether updating the cached rom header & settings
// for given filename succeeds
bool CoreUpdateCachedRomHeaderAndSettings(std::filesystem::path file);
#endif // CORE_INTERNAL

// returns whether clearing rom header & settings cache
// succeeds
bool CoreClearRomHeaderAndSettingsCache(void);

#endif // CORE_CACHEDROMHEADERANDSETTINGS_HPP
