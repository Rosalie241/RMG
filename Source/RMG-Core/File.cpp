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
#include "File.hpp"

#include "Library.hpp"
#include "Error.hpp"

#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#else
#include <sys/stat.h>
#endif

//
// Exported Functions
//

CORE_EXPORT bool CoreReadFile(std::filesystem::path file, std::vector<char>& outBuffer)
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

CORE_EXPORT bool CoreWriteFile(std::filesystem::path file, std::vector<char>& buffer)
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

CORE_EXPORT CoreFileTime CoreGetFileTime(std::filesystem::path file)
{
#ifdef _WIN32
    BOOL ret;
    HANDLE file_handle;
    FILETIME file_time;
    ULARGE_INTEGER ularge_int;

    file_handle = CreateFileW(file.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    ret = GetFileTime(file_handle, nullptr, nullptr, &file_time);
    if (ret != TRUE)
    {
        return 0;
    }

    ret = CloseHandle(file_handle);
    if (ret != TRUE)
    {
        return 0;
    }

    ularge_int.LowPart  = file_time.dwLowDateTime;
    ularge_int.HighPart = file_time.dwHighDateTime;

    return ularge_int.QuadPart;
#else // Linux
    int ret;
    struct stat file_stat;

    ret = stat(file.string().c_str(), &file_stat);
    if (ret != 0)
    {
        return 0;
    }

    return file_stat.st_mtime;
#endif
}
