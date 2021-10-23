/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef OSAL_DYNLIB_HPP
#define OSAL_DYNLIB_HPP

#include <iostream>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE osal_dynlib_lib_handle;
typedef FARPROC osal_dynlib_lib_sym;
#else // Unix
typedef void*   osal_dynlib_lib_handle;
typedef void*   osal_dynlib_lib_sym;
#endif // _WIN32

// returns library handle for given filename,
// return nullptr when invalid library
osal_dynlib_lib_handle osal_dynlib_open(char *);

// retrieves symbol handle for given library 
// handle, returns nullptr when not found
osal_dynlib_lib_sym osal_dynlib_sym(osal_dynlib_lib_handle, const char *);

// closes library handle
void osal_dynlib_close(osal_dynlib_lib_handle);

// returns error message
std::string osal_dynlib_strerror(void);

#endif // OSAL_DYNLIB_HPP