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

#include <dlfcn.h>

osal_dynlib_lib_handle osal_dynlib_open(std::filesystem::path path)
{
    return dlopen(path.string().c_str(), RTLD_LAZY);
}

osal_dynlib_lib_sym osal_dynlib_sym(osal_dynlib_lib_handle handle, const char* symbol)
{
    return dlsym(handle, symbol);
}

void osal_dynlib_close(osal_dynlib_lib_handle handle)
{
    dlclose(handle);
}

std::string osal_dynlib_strerror(void)
{
    return std::string(dlerror());
}
