/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_FILE_HPP
#define CORE_FILE_HPP

#include <filesystem>
#include <vector>

typedef uint64_t CoreFileTime;

// attempts to read the file into the buffer
bool CoreReadFile(std::filesystem::path file, std::vector<char>& outBuffer);

// attempts to write the buffer to file
bool CoreWriteFile(std::filesystem::path file, std::vector<char>& buffer);

// attempts to retrieve the file time
CoreFileTime CoreGetFileTime(std::filesystem::path file);

#endif // CORE_FILE_HPP