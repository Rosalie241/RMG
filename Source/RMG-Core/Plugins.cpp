/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Plugins.hpp"
#include "Error.hpp"
#include "RomSettings.hpp"
#include "Settings/Settings.hpp"

#include "m64p/PluginApi.hpp"
#include "osal/osal_dynlib.hpp"

#include "m64p/PluginApi.cpp"
#include "m64p/Api.hpp"

#include <filesystem>
#include <string>

//
// Local Variables
//

static m64p::PluginApi l_Plugins[(int)CorePluginType::Input];
static std::string     l_PluginFiles[(int)CorePluginType::Input];

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

    for (const auto& entry : std::filesystem::recursive_directory_iterator("Plugin"))
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
    std::string            error;
    std::string            settingValue;
    m64p::PluginApi*       plugin;
    osal_dynlib_lib_handle handle;
    m64p_error             ret;

    SettingsID settings[] = 
    {
        SettingsID::Core_RSP_Plugin,
        SettingsID::Core_GFX_Plugin,
        SettingsID::Core_AUDIO_Plugin,
        SettingsID::Core_INPUT_Plugin
    };

    for (int i = 0; i < 4; i++)
    {
        settingValue = CoreSettingsGetStringValue(settings[i]);
        if (settingValue.empty() ||
            !std::filesystem::is_regular_file(settingValue))
        { // skip invalid setting value
            continue;
        }

        if (settingValue != l_PluginFiles[i])
        {
            plugin = &l_Plugins[i];

            // shutdown plugin when hooked
            if (plugin->IsHooked())
            {
                ret = plugin->Shutdown();
                if (ret != M64ERR_SUCCESS)
                {
                    error = "CoreApplyPluginSettings m64p::PluginApi.Shutdown() Failed: ";
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
                error = "CoreApplyPluginSettings osal_dynlib_open Failed: ";
                error += osal_dynlib_strerror();
                CoreSetError(error);
                return false;
            }

            // attempt to hook the library
            if (!plugin->Hook(handle))
            {
                error = "CoreApplyPluginSettings m64p::PluginApi.Hook() Failed: ";
                error += plugin->GetLastError();
                CoreSetError(error);
                return false;
            }

            // attempt to start plugin
            ret = plugin->Startup(m64p::Core.GetHandle(), nullptr, nullptr);
            if (ret != M64ERR_SUCCESS)
            {
                error = "CoreApplyPluginSettings m64p::PluginApi.Startup() Failed: ";
                error += m64p::Core.ErrorMessage(ret);
                CoreSetError(error);
                return false;
            }

            l_PluginFiles[i] = settingValue;
        }
    }

    return true;
}

bool CoreApplyRomPluginSettings(void)
{
    std::string            error;
    std::string            settingValue;
    m64p::PluginApi*       plugin;
    osal_dynlib_lib_handle handle;
    m64p_error             ret;
    CoreRomSettings        romSettings;

    SettingsID settings[] = 
    {
        SettingsID::Game_RSP_Plugin,
        SettingsID::Game_GFX_Plugin,
        SettingsID::Game_AUDIO_Plugin,
        SettingsID::Game_INPUT_Plugin
    };

    if (!CoreGetCurrentDefaultRomSettings(romSettings))
    {
        return false;
    }

    for (int i = 0; i < 4; i++)
    {
        settingValue = CoreSettingsGetStringValue(settings[i], romSettings.MD5);
        if (settingValue.empty() ||
            !std::filesystem::is_regular_file(settingValue))
        { // skip invalid setting value
            continue;
        }

        if (settingValue != l_PluginFiles[i])
        {
            plugin = &l_Plugins[i];

            // shutdown plugin when hooked
            if (plugin->IsHooked())
            {
                ret = plugin->Shutdown();
                if (ret != M64ERR_SUCCESS)
                {
                    error = "CoreApplyRomPluginSettings m64p::PluginApi.Shutdown() Failed: ";
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
                error = "CoreApplyRomPluginSettings osal_dynlib_open Failed: ";
                error += osal_dynlib_strerror();
                CoreSetError(error);
                return false;
            }

            // attempt to hook the library
            if (!plugin->Hook(handle))
            {
                error = "CoreApplyRomPluginSettings m64p::PluginApi.Hook() Failed: ";
                error += plugin->GetLastError();
                CoreSetError(error);
                return false;
            }

            // attempt to start plugin
            ret = plugin->Startup(m64p::Core.GetHandle(), nullptr, nullptr);
            if (ret != M64ERR_SUCCESS)
            {
                error = "CoreApplyRomPluginSettings m64p::PluginApi.Startup() Failed: ";
                error += m64p::Core.ErrorMessage(ret);
                CoreSetError(error);
                return false;
            }

            l_PluginFiles[i] = settingValue;
        }
    }

    return true;
}

bool CoreArePluginsReady(void)
{
    std::string error;

    for (int i = 0; i < 4; i++)
    {
        if (!l_Plugins[i].IsHooked())
        {
            error = "CoreArePluginsReady Failed: ";
            error += "PluginApi::IsHooked returned false!";
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

    if (!CorePluginsHasConfig(type))
    {
        error = "CorePluginsOpenConfig Failed: ";
        error += "plugin with given type doesn't have config function!";
        CoreSetError(error);
        return false;
    }

    ret = get_plugin(type)->Config();
    if (ret != M64ERR_SUCCESS)
    {
        error = "CorePluginsOpenConfig m64p::PluginApi.Config() Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
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

    for (int i = 0; i < 4; i++)
    {
        ret = m64p::Core.AttachPlugin(plugin_types[i], get_plugin((CorePluginType)plugin_types[i])->GetHandle());
        if (ret != M64ERR_SUCCESS)
        {
            error = "CoreAttachPlugins m64p::Core.AttachPlugin() Failed: ";
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

    for (int i = 0; i < 4; i++)
    {
        ret = m64p::Core.DetachPlugin((m64p_plugin_type)(i + 1));
        if (ret != M64ERR_SUCCESS)
        {
            error = "CoreDetachPlugins m64p::Core.DetachPlugin() Failed: ";
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
                error = "CorePluginsShutdown m64p::PluginApi.Shutdown() Failed: ";
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
