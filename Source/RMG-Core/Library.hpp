/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_LIBRARY_HPP
#define CORE_LIBRARY_HPP

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE CoreLibraryHandle;
typedef FARPROC CoreLibrarySymbol;
#define CORE_LIBRARY_EXT_STR ".dll"
#else // Unix
typedef void*   CoreLibraryHandle;
typedef void*   CoreLibrarySymbol;
#define CORE_LIBRARY_EXT_STR ".so"
#endif // _WIN32

// returns library handle for given filename,
// return nullptr when invalid library
CoreLibraryHandle CoreOpenLibrary(std::filesystem::path path);

// retrieves symbol handle for given library 
// handle, returns nullptr when not found
CoreLibrarySymbol CoreGetLibrarySymbol(CoreLibraryHandle handle, const char* symbol);

// closes library handle
void CoreCloseLibrary(CoreLibraryHandle);

// returns error message
std::string CoreGetLibraryError(void);

#endif // CORE_LIBRARY_HPP
