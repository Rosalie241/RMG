/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Error.hpp"
#include "File.hpp"

#include <cstring>
#include <fstream>

//
// Exported Functions
//

bool CoreReadFile(std::filesystem::path file, std::vector<char>& outBuffer)
{
    std::string   error;
    std::ifstream fileStream;
    int           fileStreamLen;

    // attempt to open file
    fileStream.open(file, std::ios::binary);
    if (!fileStream.is_open())
    {
        error = "CoreReadFile Failed: ";
        error += "failed to open file: ";
        error += strerror(errno);
        error += " (";
        error += std::to_string(errno);
        error += ")";
        CoreSetError(error);
        return false;
    }

    // attempt to retrieve file length
    fileStream.seekg(0, fileStream.end);
    fileStreamLen = fileStream.tellg();
    fileStream.seekg(0, fileStream.beg);

    // resize buffer
    outBuffer.resize(fileStreamLen);

    // read file
    fileStream.read(outBuffer.data(), fileStreamLen);

    fileStream.close();
    return true;
}

bool CoreWriteFile(std::filesystem::path file, std::vector<char>& buffer)
{
    std::string   error;
    std::ofstream fileStream;

    // attempt to open file
    fileStream.open(file, std::ios::binary);
    if (!fileStream.is_open())
    {
        error = "CoreWriteFile Failed: ";
        error += "failed to open file: ";
        error += strerror(errno);
        error += " (";
        error += std::to_string(errno);
        error += ")";
        CoreSetError(error);
        return false;
    }

    // write buffer to file
    fileStream.write(buffer.data(), buffer.size());
    fileStream.close();
    return true;
}
