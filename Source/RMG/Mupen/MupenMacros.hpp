#ifndef MUPENMACROS_HPP
#define MUPENMACROS_HPP

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

//
// This macro binds m64p_func from m64p_handle (using DLSYM)
// if it fails, appends DLGETERRSTR() to error_message and return false
//
#define MUPEN_HOOK_FUNC(func)                                        \
    this->m64p_##func = (ptr_##func)DLSYM(this->m64p_handle, #func); \
    if (this->m64p_##func == NULL)                                   \
    {                                                                \
        this->error_message += DLGETERRSTR();                        \
        return false;                                                \
    }

//
// This macro binds plugin_name from file (using DLOPEN),
// it also binds "PluginStartup" to plugin_PluginStartup (using DLSYM) and
// executes that function (with m64p_handle),
// if it fails, appends DLGETERRSTR() to error_message and return false
//
#define MUPEN_PLUGIN_HOOK(file, name) \
    this->plugin_##name = DLOPEN(file); \
    if (this->plugin_##name == NULL) \
    { \
        this->error_message += DLGETERRSTR(); \
        return false; \
    }; \
    this->plugin_PluginStartup = (ptr_PluginStartup)DLSYM(this->plugin_##name, "PluginStartup"); \
    if (this->plugin_PluginStartup == NULL) \
    { \
        this->error_message += DLGETERRSTR(); \
        return false; \
    } \
    ret = this->plugin_PluginStartup(this->m64p_handle, NULL, NULL); \
    if (ret != M64ERR_SUCCESS) \
    { \
        this->error_message += this->m64p_CoreErrorMessage(ret); \
        return false; \
    }

#define MUPEN_PLUGIN_UNHOOK(file, name) \
    this->plugin_PluginShutdown = (ptr_PluginShutdown)DLSYM(this->plugin_##name, "PluginShutdown"); \
    if (this->plugin_PluginShutdown == NULL) \
    { \
        this->error_message += DLGETERRSTR(); \
        return false; \
    } \
    ret = this->plugin_PluginShutdown(); \
    if (ret != M64ERR_SUCCESS) \
    { \
        this->error_message += this->m64p_CoreErrorMessage(ret); \
        return false; \
    } \
    DLCLOSE(this->plugin_##name); \


//
// This macro binds "PluginConfig" to plugin_Config_name (using DLSYM)
//
#define MUPEN_PLUGIN_CONFIG_HOOK(name) \
    this->plugin_Config_##name = (ptr_PluginConfig)DLSYM(this->plugin_##name, "PluginConfig");

//
// This macro attachs plugin_name with type to mupen using m64p_CoreAttachPlugin,
// if it fails, appends m64p_CoreErrorMessage(ret) to error_message and return false
//
#define MUPEN_PLUGIN_ATTACH(type, name) \
    ret = this->m64p_CoreAttachPlugin(type, this->plugin_##name); \
    if (ret != M64ERR_SUCCESS) \
    { \
        this->error_message += this->m64p_CoreErrorMessage(ret); \
        return false; \
    }

#endif // MUPENMACROS_HPP