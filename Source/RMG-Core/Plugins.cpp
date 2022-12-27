/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Plugins.hpp"
#include "Callback.hpp"
#include "Directories.hpp"
#include "Error.hpp"
#include "Emulation.hpp"
#include "RomSettings.hpp"
#include "Settings/Settings.hpp"

#include "m64p/PluginApi.hpp"
#include "osal/osal_dynlib.hpp"

#include "m64p/PluginApi.cpp"
#include "m64p/Api.hpp"

#include <filesystem>
#include <cstring>

//
// Local Variables
//

static m64p::PluginApi l_Plugins[(int)CorePluginType::Input];
static std::string     l_PluginFiles[(int)CorePluginType::Input];
static char l_PluginContext[(int)CorePluginType::Input][20];

//
// Local Functions
//

m64p::PluginApi* get_plugin(CorePluginType type)
{
    return &l_Plugins[(int)type - 1];
}

CorePluginType get_plugin_type(m64p::PluginApi* plugin)
{
    m64p_error ret;
    m64p_plugin_type m64p_type = M64PLUGIN_NULL;

    ret = plugin->GetVersion(&m64p_type, nullptr, nullptr, nullptr, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        return CorePluginType::Invalid;
    }

    if (m64p_type < 1 || m64p_type > 4)
    {
        return CorePluginType::Invalid;
    }

    return (CorePluginType)m64p_type;
}

std::string get_plugin_name(m64p::PluginApi* plugin, std::string filename)
{
    m64p_error ret;
    const char* name = nullptr;
    
    ret = plugin->GetVersion(nullptr, nullptr, nullptr, &name, nullptr);
    if (ret != M64ERR_SUCCESS || 
        name == nullptr)
    {
        return filename;
    }

    return std::string(name);
}

std::string get_plugin_type_name(CorePluginType type)
{
    std::string name;

    switch (type)
    {
        default:
            name = "Unknown";
            break;
        case CorePluginType::Rsp:
            name = "Rsp";
            break;
        case CorePluginType::Gfx:
            name = "Gfx";
            break;
        case CorePluginType::Audio:
            name = "Audio";
            break;
        case CorePluginType::Input:
            name = "Input";
            break;
        case CorePluginType::Invalid:
            name = "Invalid";
            break;
    }

    return name + " Plugin";
}

std::string get_plugin_context_name(CorePluginType type)
{
    std::string name;

    switch (type)
    {
        default:
            name = "[UNKNOWN]";
            break;
        case CorePluginType::Rsp:
            name = "[RSP]   ";
            break;
        case CorePluginType::Gfx:
            name = "[GFX]   ";
            break;
        case CorePluginType::Audio:
            name = "[AUDIO] ";
            break;
        case CorePluginType::Input:
            name = "[INPUT] ";
            break;
    }

    return name;
}

bool apply_plugin_settings(std::string pluginSettings[4])
{
    std::string            error;
    std::string            settingValue;
    m64p::PluginApi*       plugin;
    CorePluginType         pluginType;
    osal_dynlib_lib_handle handle;
    m64p_error             ret;

    for (int i = 0; i < 4; i++)
    {
        settingValue = pluginSettings[i];
        if (settingValue.empty() ||
            !std::filesystem::is_regular_file(settingValue))
        { // skip invalid setting value
            continue;
        }

        pluginType = (CorePluginType)(i + 1);

        // copy context string to a c string using strcpy
        std::strcpy(l_PluginContext[(int)pluginType], get_plugin_context_name(pluginType).c_str());

        if (settingValue != l_PluginFiles[i])
        {
            plugin = &l_Plugins[i];

            // shutdown plugin when hooked
            if (plugin->IsHooked())
            {
                ret = plugin->Shutdown();
                if (ret != M64ERR_SUCCESS)
                {
                    error = "apply_plugin_settings (";
                    error += get_plugin_type_name(pluginType);
                    error += ")->Shutdown() Failed: ";
                    error += m64p::Core.ErrorMessage(ret);
                    CoreSetError(error);
                    return false;
                }

                // reset plugin
                plugin->Unhook();
            }

            // attempt to open the library
            handle = osal_dynlib_open(settingValue.c_str());
            if (handle == nullptr)
            {
                error = "apply_plugin_settings osal_dynlib_open Failed: ";
                error += osal_dynlib_strerror();
                CoreSetError(error);
                return false;
            }

            // attempt to hook the library
            if (!plugin->Hook(handle))
            {
                error = "apply_plugin_settings (";
                error += get_plugin_type_name(pluginType);
                error += ")->Hook() Failed: ";
                error += plugin->GetLastError();
                CoreSetError(error);
                plugin->Unhook();
                return false;
            }

            // make sure the plugin type is the expected type
            if (get_plugin_type(plugin) != pluginType)
            {
                error = "apply_plugin_settings plugin type ";
                error += get_plugin_type_name(get_plugin_type(plugin));
                error += " doesn't match expected type ";
                error += get_plugin_type_name(pluginType);
                error += "!";
                CoreSetError(error);
                plugin->Unhook();
                return false;
            }

            // attempt to start plugin
            ret = plugin->Startup(m64p::Core.GetHandle(), (void*)l_PluginContext[(int)pluginType], CoreDebugCallback);
            if (ret != M64ERR_SUCCESS)
            {
                error = "apply_plugin_settings (";
                error += get_plugin_type_name(pluginType);
                error += ")->Startup() Failed: ";
                error += m64p::Core.ErrorMessage(ret);
                CoreSetError(error);
                plugin->Shutdown();
                plugin->Unhook();
                // force a re-load next time,
                // because this plugin isn't
                // the one we've stored in 
                // l_PluginFiles[i]
                l_PluginFiles[i].clear();
                return false;
            }

            l_PluginFiles[i] = settingValue;
        }
    }

    return true;
}

//
// Exported Functions
//

std::vector<CorePlugin> CoreGetAllPlugins(void)
{
    std::vector<CorePlugin> plugins;
    std::string             plugin_name;
    CorePluginType          plugin_type;
    osal_dynlib_lib_handle  handle;
    m64p::PluginApi         plugin;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(CoreGetPluginDirectory()))
    {
        std::string path = entry.path().string();
        if (!entry.is_directory() &&
            path.ends_with(OSAL_DYNLIB_LIB_EXT_STR))
        {
            handle = osal_dynlib_open(path.c_str());
            if (handle == nullptr || !plugin.Hook(handle))
            { // skip invalid libs
                continue;
            }

            plugin_name = get_plugin_name(&plugin, entry.path().filename().string());
            plugin_type = get_plugin_type(&plugin);

            plugin.Unhook();
            osal_dynlib_close(handle);

            if (plugin_type == CorePluginType::Invalid)
            { // skip unsupported plugin types
                continue;
            }

            CorePlugin corePlugin = {path, plugin_name, plugin_type};
            plugins.emplace_back(corePlugin);
        }
    }

    return plugins;
}

bool CoreApplyPluginSettings(void)
{
    std::string settings[] = 
    {
        CoreSettingsGetStringValue(SettingsID::Core_RSP_Plugin),
        CoreSettingsGetStringValue(SettingsID::Core_GFX_Plugin),
        CoreSettingsGetStringValue(SettingsID::Core_AUDIO_Plugin),
        CoreSettingsGetStringValue(SettingsID::Core_INPUT_Plugin)
    };

    return apply_plugin_settings(settings);
}

bool CoreApplyRomPluginSettings(void)
{
    CoreRomSettings          romSettings;

    if (!CoreGetCurrentDefaultRomSettings(romSettings))
    {
        return false;
    }

    std::string settings[] =
    {
        CoreSettingsGetStringValue(SettingsID::Game_RSP_Plugin, romSettings.MD5),
        CoreSettingsGetStringValue(SettingsID::Game_GFX_Plugin, romSettings.MD5),
        CoreSettingsGetStringValue(SettingsID::Game_AUDIO_Plugin, romSettings.MD5),
        CoreSettingsGetStringValue(SettingsID::Game_INPUT_Plugin, romSettings.MD5)
    };

    return apply_plugin_settings(settings);
}

bool CoreArePluginsReady(void)
{
    std::string error;

    for (int i = 0; i < 4; i++)
    {
        if (!l_Plugins[i].IsHooked())
        {
            error = "CoreArePluginsReady Failed: ";
            error += "(";
            error += get_plugin_type_name((CorePluginType)(i + 1));
            error += ")->IsHooked returned false!";
            CoreSetError(error);
            return false;
        }
    }

    return true;
}

bool CorePluginsHasConfig(CorePluginType type)
{
    return get_plugin(type)->Config != nullptr;
}

bool CorePluginsOpenConfig(CorePluginType type)
{
    std::string error;
    m64p_error ret;
    bool resumeEmulation = false;

    if (!CorePluginsHasConfig(type))
    {
        error = "CorePluginsOpenConfig Failed: ";
        error += get_plugin_type_name(type);
        error += " doesn't have config function!";
        CoreSetError(error);
        return false;
    }

    // try to pause emulation,
    // when emulation is running
    // and try to resume afterwards
    if (CoreIsEmulationRunning())
    {
        // only resume emulation
        // after running the config function
        // when pausing succeeds
        resumeEmulation = CorePauseEmulation();
    }

    ret = get_plugin(type)->Config();
    if (ret != M64ERR_SUCCESS)
    {
        error = "CorePluginsOpenConfig (";
        error += get_plugin_type_name(type);
        error += ")->Config() Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    // try to resume emulation when needed
    if (resumeEmulation)
    {
        CoreResumeEmulation();
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreAttachPlugins(void)
{
    std::string error;
    m64p_error ret;
    m64p_plugin_type plugin_types[] =
    {
        M64PLUGIN_GFX,
        M64PLUGIN_AUDIO,
        M64PLUGIN_INPUT,
        M64PLUGIN_RSP
    };

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    for (int i = 0; i < 4; i++)
    {
        ret = m64p::Core.AttachPlugin(plugin_types[i], get_plugin((CorePluginType)plugin_types[i])->GetHandle());
        if (ret != M64ERR_SUCCESS)
        {
            error = "CoreAttachPlugins m64p::Core.AttachPlugin(";
            error += get_plugin_type_name((CorePluginType)plugin_types[i]);
            error += ") Failed: ";
            error += m64p::Core.ErrorMessage(ret);
            CoreSetError(error);
            break;
        }
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreDetachPlugins(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    for (int i = 0; i < 4; i++)
    {
        ret = m64p::Core.DetachPlugin((m64p_plugin_type)(i + 1));
        if (ret != M64ERR_SUCCESS)
        {
            error = "CoreDetachPlugins m64p::Core.DetachPlugin(";
            error += get_plugin_type_name((CorePluginType)(i + 1));
            error += ") Failed: ";
            error += m64p::Core.ErrorMessage(ret);
            CoreSetError(error);
            break;
        }
    }
   
    return ret == M64ERR_SUCCESS;
}

bool CorePluginsShutdown(void)
{
    std::string      error;
    m64p::PluginApi* plugin;
    m64p_error       ret;

    for (int i = 0; i < 4; i++)
    {
        plugin = &l_Plugins[i];

        // shutdown plugin when hooked
        if (plugin->IsHooked())
        {
            ret = plugin->Shutdown();
            if (ret != M64ERR_SUCCESS)
            {
                error = "CorePluginsShutdown (";
                error += get_plugin_type_name((CorePluginType)(i + 1));
                error += ")->Shutdown() Failed: ";
                error += m64p::Core.ErrorMessage(ret);
                CoreSetError(error);
                break;
            }

            // reset plugin
            plugin->Unhook();
        }
    }

    return ret == M64ERR_SUCCESS;
}
