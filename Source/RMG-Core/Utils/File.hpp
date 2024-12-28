/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_FILE_HPP
#define CORE_FILE_HPP

#ifdef CORE_INTERNAL

#include <filesystem>
#include <vector>

// attempts to read the file into the buffer
bool CoreReadFile(std::filesystem::path file, std::vector<char>& outBuffer);

// attempts to write the buffer to file
bool CoreWriteFile(std::filesystem::path file, std::vector<char>& buffer);


#endif // CORE_INTERNAL

#endif // CORE_FILE_HPP