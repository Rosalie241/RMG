/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_ARCHIVE_HPP
#define CORE_ARCHIVE_HPP

#include <filesystem>
#include <vector>

// attempts to read the ROM/disk in a zip file into outBuffer
bool CoreReadZipFile(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer);

// attempts to read the ROM/disk in a 7zip file into outBuffer
bool CoreRead7zipFile(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer);

// attempts to read the ROM/disk in a supported archive file into outBuffer
bool CoreReadArchiveFile(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer);

// attempts to unzip the file to path
bool CoreUnzip(std::filesystem::path file, std::filesystem::path path);

#endif // CORE_ARCHIVE_HPP
