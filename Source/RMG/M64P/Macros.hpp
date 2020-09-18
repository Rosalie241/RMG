/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef M64P_MACROS_HPP
#define M64P_MACROS_HPP

#ifdef _WIN32
#include <windows.h>
#define DLOPEN(lib) LoadLibrary(lib)
#define DLCLOSE(lib) FreeLibrary(lib)
#define DLSYM(lib, func) GetProcAddress(lib, func)
#define DLGETERRSTR() GetLastError()
#else
#include <dlfcn.h>
#define DLOPEN(lib) dlopen(lib, RTLD_LAZY)
#define DLCLOSE(lib) dlclose(lib)
#define DLSYM(lib, func) dlsym(lib, func)
#define DLGETERRSTR() dlerror()
#endif

#define HOOK_FUNC_OPT(handle, prevar, var) \
    this->var = (ptr_##prevar##var)DLSYM(handle, #prevar#var);
    
#define HOOK_FUNC(handle, prevar, var) \
    this->var = (ptr_##prevar##var)DLSYM(handle, #prevar#var); \
    if (this->var == nullptr) \
    { \
        this->error_Message += DLGETERRSTR(); \
        return false; \
    }

#endif // M64P_MACROS_HPP
