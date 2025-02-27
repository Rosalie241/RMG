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
#include "CachedRomHeaderAndSettings.hpp"
#include "Directories.hpp"
#include "RomSettings.hpp"
#include "Emulation.hpp"
#include "RomHeader.hpp"
#include "Callback.hpp"
#include "Settings.hpp"
#include "Library.hpp"
#include "Plugins.hpp"
#include "Error.hpp"

#include "m64p/PluginApi.hpp"
#include "m64p/Api.hpp"

#include <filesystem>
#include <algorithm>
#include <cstring>
#include <array>

//
// Local Defines
//

#define NUM_PLUGIN_TYPES 4

//
// Local Variables
//

static m64p::PluginApi l_Plugins[NUM_PLUGIN_TYPES];
static std::string     l_PluginFiles[NUM_PLUGIN_TYPES];
static char l_PluginContext[NUM_PLUGIN_TYPES][20];

//
// Local Functions
//

static m64p::PluginApi* get_plugin(CorePluginType type)
{
    if (type == CorePluginType::Invalid ||
        (int)type < 0 || (int)type > 4)
    {
        return nullptr;
    }

    return &l_Plugins[(int)type - 1];
}

static CorePluginType get_plugin_type(m64p::PluginApi* plugin)
{
    m64p_error ret;
    m64p_plugin_type m64p_type = M64PLUGIN_NULL;

    ret = plugin->GetVersion(&m64p_type, nullptr, nullptr, nullptr, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        return CorePluginType::Invalid;
    }

    if (m64p_type < (int)CorePluginType::Rsp || 
        m64p_type > (int)CorePluginType::Input)
    {
        return CorePluginType::Invalid;
    }

    return (CorePluginType)m64p_type;
}

static std::string get_plugin_name(m64p::PluginApi* plugin, const std::string& filename)
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

static std::string get_plugin_type_name(CorePluginType type)
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
    }

    return name + " Plugin";
}

static std::string get_plugin_context_name(CorePluginType type)
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

static std::string get_plugin_path(CorePluginType type, std::string settingsValue)
{
    std::string pluginPath;
    std::string path;
    std::string typeName;

    // return an empty string when the value is empty
    if (settingsValue.empty())
    {
        return std::string();
    }

    pluginPath = CoreGetPluginDirectory().string();

    // if the full plugin path is in the settings value,
    // we know it's the old type
    if (settingsValue.find(pluginPath) != std::string::npos)
    {
        return settingsValue;
    }

    switch (type)
    {
    case CorePluginType::Rsp:
        typeName = "RSP";
        break;
    case CorePluginType::Gfx:
        typeName = "GFX";
        break;
    case CorePluginType::Audio:
        typeName = "Audio";
        break;
    case CorePluginType::Input:
        typeName = "Input";
        break;
    default:
        return path;
    }

    path = pluginPath;
    path += CORE_DIR_SEPERATOR_STR;
    path += typeName;
    path += CORE_DIR_SEPERATOR_STR;
    path += settingsValue;

    return path;
}

static bool apply_plugin_settings(const std::array<std::string, 4>& pluginSettings)
{
    std::string       error;
    std::string       settingValue;
    m64p::PluginApi*  plugin;
    CorePluginType    pluginType;
    CoreLibraryHandle handle;
    m64p_error        ret;

    for (int i = 0; i < NUM_PLUGIN_TYPES; i++)
    {
        pluginType = (CorePluginType)(i + 1);
        settingValue = get_plugin_path(pluginType, pluginSettings[i]);
        if (settingValue.empty())
        { // skip invalid setting value
            continue;
        }

        // copy context string to a c string using strcpy
        std::strcpy(l_PluginContext[i], get_plugin_context_name(pluginType).c_str());

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

            // ensure library file exists
            if (!std::filesystem::is_regular_file(settingValue))
            {
                // force a re-load next time,
                // because we've unhooked
                // the existing one
                l_PluginFiles[i].clear();
                continue;
            }

            // attempt to open the library
            handle = CoreOpenLibrary(settingValue.c_str());
            if (handle == nullptr)
            {
                error = "apply_plugin_settings CoreOpenLibrary Failed: ";
                error += CoreGetLibraryError();
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
            ret = plugin->Startup(m64p::Core.GetHandle(), (void*)l_PluginContext[i], CoreDebugCallback);
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

            CoreAddCallbackMessage(CoreDebugMessageType::Info, 
                "Loaded plugin " + std::filesystem::path(settingValue).filename().string());
        }
    }

    return true;
}

static bool open_plugin_config(CorePluginType type, void* parent, bool romConfig, std::filesystem::path file)
{
    std::string error;
    m64p_error ret;
    bool resumeEmulation = false;
    m64p::PluginApi* plugin;
    std::string functionName;
    CoreRomHeader romHeader;
    CoreRomSettings romSettings;

    if (!romConfig && !CorePluginsHasConfig(type))
    {
        error = "open_plugin_config Failed: ";
        error += get_plugin_type_name(type);
        error += " doesn't have the config or config2 function!";
        CoreSetError(error);
        return false;
    }
    else if (romConfig && !CorePluginsHasROMConfig(type))
    {
        error = "open_plugin_config Failed: ";
        error += get_plugin_type_name(type);
        error += " doesn't support ROM specific configuration!";
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

    if (romConfig && !file.empty())
    {
        // try to retrieve cached rom header & settings
        if (!CoreGetCachedRomHeaderAndSettings(file, nullptr, &romHeader, nullptr, &romSettings))
        {
            return false;
        }
    }

    plugin = get_plugin(type);
    if (plugin == nullptr)
    {
        error = "open_plugin_config Failed: ";
        error += get_plugin_type_name(type);
        error += " isn't a valid plugin type!";
        CoreSetError(error);
        return false;
    }

    // check if the plugin has the Config2
    // or Config function, the Config2 function
    // has priority
    if (plugin->Config2 != nullptr)
    {
        ret = plugin->Config2(parent, romConfig ? 1 : 0, &romHeader, &romSettings);
        functionName = "Config2";
    }
    else
    {
        ret = plugin->Config(parent);
        functionName = "Config";
    }

    if (ret != M64ERR_SUCCESS)
    {
        error = "open_plugin_config (";
        error += get_plugin_type_name(type);
        error += ")->";
        error += functionName;
        error += "() Failed: ";
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

//
// Exported Functions
//

CORE_EXPORT std::vector<CorePlugin> CoreGetAllPlugins(void)
{
    std::vector<CorePlugin> plugins;
    std::string             plugin_name;
    CorePluginType          plugin_type;
    CoreLibraryHandle       handle;
    m64p::PluginApi         plugin;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(CoreGetPluginDirectory()))
    {
        std::string path = entry.path().string();
        std::string file = entry.path().filename().string();
        if (!entry.is_directory() &&
            path.ends_with(CORE_LIBRARY_EXT_STR))
        {
            handle = CoreOpenLibrary(path.c_str());
            if (handle == nullptr || !plugin.Hook(handle))
            { // skip invalid libs
                continue;
            }

            plugin_name = get_plugin_name(&plugin, entry.path().filename().string());
            plugin_type = get_plugin_type(&plugin);

            plugin.Unhook();
            CoreCloseLibrary(handle);

            if (plugin_type == CorePluginType::Invalid)
            { // skip unsupported plugin types
                continue;
            }

            CorePlugin corePlugin = {file, plugin_name, plugin_type};
            plugins.emplace_back(corePlugin);
        }
    }

    std::sort(plugins.begin(), plugins.end(), [](CorePlugin& a, CorePlugin& b)
    {
        return a.Name > b.Name;
    });

    return plugins;
}

CORE_EXPORT bool CoreApplyPluginSettings(void)
{
    const std::array<std::string, 4> settings = 
    {
        CoreSettingsGetStringValue(SettingsID::Core_RSP_Plugin),
        CoreSettingsGetStringValue(SettingsID::Core_GFX_Plugin),
        CoreSettingsGetStringValue(SettingsID::Core_AUDIO_Plugin),
        CoreSettingsGetStringValue(SettingsID::Core_INPUT_Plugin)
    };

    return apply_plugin_settings(settings);
}

CORE_EXPORT bool CoreApplyRomPluginSettings(void)
{
    CoreRomSettings romSettings;

    if (!CoreGetCurrentDefaultRomSettings(romSettings))
    {
        return false;
    }

    const std::array<std::string, 4> settings =
    {
        CoreSettingsGetStringValue(SettingsID::Game_RSP_Plugin, romSettings.MD5),
        CoreSettingsGetStringValue(SettingsID::Game_GFX_Plugin, romSettings.MD5),
        CoreSettingsGetStringValue(SettingsID::Game_AUDIO_Plugin, romSettings.MD5),
        CoreSettingsGetStringValue(SettingsID::Game_INPUT_Plugin, romSettings.MD5)
    };

    return apply_plugin_settings(settings);
}

CORE_EXPORT bool CoreArePluginsReady(void)
{
    std::string error;

    for (int i = 0; i < NUM_PLUGIN_TYPES; i++)
    {
        if (!l_Plugins[i].IsHooked())
        {
            error = "CoreArePluginsReady Failed: ";
            error += "(";
            error += get_plugin_type_name((CorePluginType)(i + 1));
            error += ")->IsHooked() returned false!";
            CoreSetError(error);
            return false;
        }
    }

    return true;
}

CORE_EXPORT bool CorePluginsHasConfig(CorePluginType type)
{
    std::string error;
    m64p::PluginApi* plugin;

    plugin = get_plugin(type);
    if (plugin == nullptr)
    {
        error = "CorePluginsHasConfig Failed: ";
        error += get_plugin_type_name(type);
        error += " isn't a valid plugin type!";
        CoreSetError(error);
        return false;
    }

    return plugin->Config != nullptr ||
            plugin->Config2 != nullptr;
}

CORE_EXPORT bool CorePluginsOpenConfig(CorePluginType type, void* parent)
{
    return open_plugin_config(type, parent, false, "");
}

CORE_EXPORT bool CorePluginsHasROMConfig(CorePluginType type)
{
    m64p::PluginApi* plugin;

    plugin = get_plugin(type);

    return plugin->Config2 != nullptr &&
            plugin->Config2HasRomConfig != nullptr &&
            plugin->Config2HasRomConfig() > 0;
}

CORE_EXPORT bool CorePluginsOpenROMConfig(CorePluginType type, void* parent, std::filesystem::path file)
{
    return open_plugin_config(type, parent, true, file);
}

CORE_EXPORT bool CoreAttachPlugins(void)
{
    std::string error;
    m64p_error ret;
    const m64p_plugin_type plugin_types[] =
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

    for (int i = 0; i < NUM_PLUGIN_TYPES; i++)
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

CORE_EXPORT bool CoreDetachPlugins(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    for (int i = 0; i < NUM_PLUGIN_TYPES; i++)
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

CORE_EXPORT bool CorePluginsShutdown(void)
{
    std::string      error;
    m64p::PluginApi* plugin;
    m64p_error       ret;

    for (int i = 0; i < NUM_PLUGIN_TYPES; i++)
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
