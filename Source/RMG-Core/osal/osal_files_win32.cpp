/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "osal_files.hpp"

#include <windows.h>
#include <fileapi.h>

osal_files_file_time osal_files_get_file_time(std::string file)
{
    BOOL ret;
    HANDLE file_handle;
    FILETIME file_time;

    file_handle = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    ret = GetFileTime(file_handle, nullptr, nullptr, &file_time);
    if (ret != TRUE)
    {
        return -1;
    }

    ret = CloseHandle(file_handle);
    if (ret != TRUE)
    {
        return -1;
    }

    // this is probably incorrect but it doesn't matter
    // for our use-case
    return file_time.dwLowDateTime + file_time.dwHighDateTime;
}
