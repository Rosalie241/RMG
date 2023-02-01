/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_MEDIALOADER_HPP
#define CORE_MEDIALOADER_HPP

#include <filesystem>

// sets up the medialoader
bool CoreSetupMediaLoader(void);

// resets media loader state
void CoreResetMediaLoader(void);

// sets the media loader disk file
void CoreMediaLoaderSetDiskFile(std::filesystem::path disk);

#endif // CORE_MEDIALOADER_HPP
