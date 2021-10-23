/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "osal_dynlib.hpp"

osal_dynlib_lib_handle osal_dynlib_open(const char* file)
{
    return LoadLibrary(file);
}

osal_dynlib_lib_sym osal_dynlib_sym(osal_dynlib_lib_handle handle, const char *sym)
{
    return GetProcAddress(handle, sym);
}

void osal_dynlib_close(osal_dynlib_lib_handle handle)
{
    FreeLibrary(handle);
}

std::string osal_dynlib_strerror(void)
{
    DWORD err = GetLastError();
    LPSTR buf = nullptr;

    size_t size =
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);

    std::string msg(buf, size);

    LocalFree(buf);

    return msg;
}
