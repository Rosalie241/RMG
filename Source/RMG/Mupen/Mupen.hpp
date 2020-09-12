#ifndef MUPEN_HPP
#define MUPEN_HPP

#include <QString>

#include "api/m64p_frontend.h"
#include "api/m64p_common.h"
#include "api/m64p_custom.h"
#include "api/version.h"
#include "../Config.hpp"
#include "VidExt.hpp"

enum PluginType
{
    Rsp,
    Gfx,
    Audio,
    Input
};

class Mupen
{
public:
    Mupen();
    ~Mupen();
    bool Init();
    bool Setup();
    bool SetupVidExt(VidExtFuncs funcs);
    
    bool OpenRom(QString);

    bool HasPluginConfig(PluginType);
    bool OpenPluginConfig(PluginType);

    bool GetRomInfo(QString, m64p_rom_header*, m64p_rom_settings*);

    QString GetLastError();

private:
    QString error_message;

    m64p_dynlib_handle m64p_handle;

    ptr_CoreStartup m64p_CoreStartup;
    ptr_CoreShutdown m64p_CoreShutdown;
    ptr_CoreAttachPlugin m64p_CoreAttachPlugin;
    ptr_CoreDetachPlugin m64p_CoreDetachPlugin;
    ptr_CoreDoCommand m64p_CoreDoCommand;
    ptr_CoreOverrideVidExt m64p_CoreOverrideVidExt;
    ptr_CoreAddCheat m64p_CoreAddCheat;
    ptr_CoreCheatEnabled m64p_CoreCheatEnabled;
    ptr_CoreGetRomSettings m64p_CoreGetRomSettings;
    ptr_CoreErrorMessage m64p_CoreErrorMessage;

    bool core_Hooked;
    bool core_Hook();
    bool core_Init();
    bool core_Setup();

    m64p_dynlib_handle plugin_Rsp;
    m64p_dynlib_handle plugin_Gfx;
    m64p_dynlib_handle plugin_Audio;
    m64p_dynlib_handle plugin_Input;

    ptr_PluginConfig plugin_Config_Rsp;
    ptr_PluginConfig plugin_Config_Gfx;
    ptr_PluginConfig plugin_Config_Audio;
    ptr_PluginConfig plugin_Config_Input;
    ptr_PluginConfig plugin_Config_Get(PluginType);

    ptr_PluginStartup plugin_PluginStartup;
    ptr_PluginShutdown plugin_PluginShutdown;

    bool plugin_Hook();
    bool plugin_Unhook();
    bool plugin_Attach();
    bool plugin_Detach();

    VidExt m64p_VidExt;
};

#endif // MUPEN_HPP
