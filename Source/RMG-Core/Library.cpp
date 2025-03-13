/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Library.hpp"

#ifndef _WIN32
#include <dlfcn.h>
#endif

//
// Exported Functions
//

CORE_EXPORT CoreLibraryHandle CoreOpenLibrary(std::filesystem::path path)
{
#ifdef _WIN32
    return LoadLibraryW(path.wstring().c_str());
#else // Linux
    return dlopen(path.string().c_str(), RTLD_LAZY);
#endif
}

CORE_EXPORT CoreLibrarySymbol CoreGetLibrarySymbol(CoreLibraryHandle handle, const char* symbol)
{
#ifdef _WIN32
    return GetProcAddress(handle, symbol);
#else
    return dlsym(handle, symbol);
#endif // Linux
}

CORE_EXPORT void CoreCloseLibrary(CoreLibraryHandle handle)
{
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif // Linux
}

CORE_EXPORT std::string CoreGetLibraryError(void)
{
#ifdef _WIN32
    DWORD err = GetLastError();
    LPSTR buf = nullptr;

    size_t size =
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);

    std::string msg(buf, size);

    LocalFree(buf);

    return msg;
#else // Linux
    return std::string(dlerror());
#endif
}
