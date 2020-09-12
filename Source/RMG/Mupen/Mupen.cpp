#include "Mupen.hpp"
#include "MupenMacros.hpp"
#include <QFile>

Mupen::Mupen()
{
}

Mupen::~Mupen()
{
    if (this->core_Hooked)
    {
        this->plugin_Unhook();
        this->m64p_CoreShutdown();
        DLCLOSE(this->m64p_handle);
    }
}

bool Mupen::Init()
{
    if (!this->core_Hook())
        return false;

    if (!this->core_Init())
        return false;

    return true;
}

bool Mupen::Setup()
{
    return this->plugin_Hook();
}

bool Mupen::SetupVidExt(VidExtFuncs funcs)
{
    m64p_error ret;

    m64p_VidExt.VidExt_Register(funcs);

    ret = m64p_CoreOverrideVidExt(m64p_VidExt.VidExt_GetFuncs());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_message = "Mupen::SetupVidExt Failed: ";
        this->error_message += m64p_CoreErrorMessage(ret);
        return false;
    }

    return true;
}

bool Mupen::OpenRom(QString file)
{
    m64p_error ret;
    QFile qFile(file);
    qFile.open(QIODevice::ReadOnly);

    ret = m64p_CoreDoCommand(M64CMD_ROM_OPEN, qFile.size(), (void *)(const void *)qFile.readAll());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_message = "Mupen::OpenRom Failed: ";
        this->error_message += m64p_CoreErrorMessage(ret);
        return false;
    }

    if (!this->plugin_Attach())
        return false;

    ret = m64p_CoreDoCommand(M64CMD_EXECUTE, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_message = "Mupen::OpenRom Failed: ";
        this->error_message += m64p_CoreErrorMessage(ret);
        return false;
    }

    return true;
}

bool Mupen::HasPluginConfig(PluginType type)
{
    ptr_PluginConfig pluginConfig = this->plugin_Config_Get(type);

    return pluginConfig != nullptr;
}

bool Mupen::OpenPluginConfig(PluginType type)
{
    ptr_PluginConfig pluginConfig = this->plugin_Config_Get(type);

    if (pluginConfig == nullptr)
        return false;

    return pluginConfig() == M64ERR_SUCCESS;
}

#include <iostream>
#include <QDataStream>

bool Mupen::GetRomInfo(QString file, m64p_rom_header* header, m64p_rom_settings* settings)
{
    m64p_error ret;

    QFile qFile(file);
    
    if (!qFile.open(QIODevice::ReadOnly))
        return false;

    ret = m64p_CoreDoCommand(M64CMD_ROM_OPEN, qFile.size(), (void *)(const void *)qFile.readAll());
    if (ret != M64ERR_SUCCESS)
    {
        std::cout << "m64p_CoreDoCommand M64CMD_ROM_OPEN Failed: " << m64p_CoreErrorMessage(ret) << std::endl;
        return false;
    }

    ret = m64p_CoreDoCommand(M64CMD_ROM_GET_HEADER, sizeof(m64p_rom_header), header);
    if (ret != M64ERR_SUCCESS)
    {
        std::cout << "m64p_CoreDoCommand M64CMD_ROM_GET_HEADER Failed: " << m64p_CoreErrorMessage(ret) << std::endl;
        return false;
    }

    ret = m64p_CoreDoCommand(M64CMD_ROM_GET_SETTINGS, sizeof(m64p_rom_settings), settings);
    if (ret != M64ERR_SUCCESS)
    {
        std::cout << "m64p_CoreDoCommand M64CMD_ROM_GET_SETTINGS Failed: " << m64p_CoreErrorMessage(ret) << std::endl;
        return false;
    }

    ret = m64p_CoreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        std::cout << "m64p_CoreDoCommand M64CMD_ROM_CLOSE Failed: " << m64p_CoreErrorMessage(ret) << std::endl;
        return false;
    }
    
    qFile.close();

    return true;
}

QString Mupen::GetLastError()
{
    return this->error_message;
}

bool Mupen::core_Hook()
{
    this->m64p_handle = DLOPEN(MUPEN_CORE_FILE);

    if (!this->m64p_handle)
    {
        this->error_message = "Mupen::core_Hook: DLOPEN Failed: " + QString::fromStdString(DLGETERRSTR());
        return false;
    }

    this->core_Hooked = false;
    this->error_message = "Mupen::core_Hook: DLSYM Failed: ";

    MUPEN_HOOK_FUNC(CoreStartup);
    MUPEN_HOOK_FUNC(CoreShutdown);
    MUPEN_HOOK_FUNC(CoreAttachPlugin);
    MUPEN_HOOK_FUNC(CoreDetachPlugin);
    MUPEN_HOOK_FUNC(CoreDetachPlugin);
    MUPEN_HOOK_FUNC(CoreDoCommand);
    MUPEN_HOOK_FUNC(CoreOverrideVidExt);
    MUPEN_HOOK_FUNC(CoreAddCheat);
    MUPEN_HOOK_FUNC(CoreCheatEnabled);
    MUPEN_HOOK_FUNC(CoreGetRomSettings);
    MUPEN_HOOK_FUNC(CoreErrorMessage);

    this->core_Hooked = true;
    return true;
}

bool Mupen::core_Init()
{
    m64p_error ret;

    ret = m64p_CoreStartup(FRONTEND_API_VERSION, MUPEN_CONFIG_DIR, MUPEN_DATA_DIR, NULL, NULL, NULL, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_message = "Mupen::core_Init: m64p_CoreStartup Failed: ";
        this->error_message += this->m64p_CoreErrorMessage(ret);
        return false;
    }

    return true;
}

bool Mupen::core_Setup()
{
    return true;
}

ptr_PluginConfig Mupen::plugin_Config_Get(PluginType type)
{
    ptr_PluginConfig pluginConfig;

    switch (type)
    {
    case PluginType::Rsp:
        pluginConfig = this->plugin_Config_Rsp;
        break;
    case PluginType::Gfx:
        pluginConfig = this->plugin_Config_Gfx;
        break;
    case PluginType::Audio:
        pluginConfig = this->plugin_Config_Audio;
        break;
    case PluginType::Input:
        pluginConfig = this->plugin_Config_Input;
        break;
    default:
        pluginConfig = nullptr;
        break;
    }

    return pluginConfig;
}

bool Mupen::plugin_Hook()
{
    m64p_error ret;

    this->error_message = "Mupen::plugin_Hook Failed: ";

    MUPEN_PLUGIN_HOOK(MUPEN_PLUGIN_RSP, Rsp);
    MUPEN_PLUGIN_HOOK(MUPEN_PLUGIN_GFX, Gfx);
    MUPEN_PLUGIN_HOOK(MUPEN_PLUGIN_AUDIO, Audio);
    MUPEN_PLUGIN_HOOK(MUPEN_PLUGIN_INPUT, Input);

    MUPEN_PLUGIN_CONFIG_HOOK(Rsp);
    MUPEN_PLUGIN_CONFIG_HOOK(Gfx);
    MUPEN_PLUGIN_CONFIG_HOOK(Audio);
    MUPEN_PLUGIN_CONFIG_HOOK(Input);

    return true;
}

bool Mupen::plugin_Unhook()
{
    m64p_error ret;
    this->error_message = "Mupen::plugin_Unhook Failed: ";

    MUPEN_PLUGIN_UNHOOK(MUPEN_PLUGIN_RSP, Rsp);
    MUPEN_PLUGIN_UNHOOK(MUPEN_PLUGIN_GFX, Gfx);
    MUPEN_PLUGIN_UNHOOK(MUPEN_PLUGIN_AUDIO, Audio);
    MUPEN_PLUGIN_UNHOOK(MUPEN_PLUGIN_INPUT, Input);

    return true;
}

bool Mupen::plugin_Attach()
{
    m64p_error ret;

    this->error_message = "Mupen::plugin_Attach Failed: ";

    MUPEN_PLUGIN_ATTACH(M64PLUGIN_GFX, Gfx);
    MUPEN_PLUGIN_ATTACH(M64PLUGIN_AUDIO, Audio);
    MUPEN_PLUGIN_ATTACH(M64PLUGIN_INPUT, Input);
    MUPEN_PLUGIN_ATTACH(M64PLUGIN_RSP, Rsp);

    return true;
}
