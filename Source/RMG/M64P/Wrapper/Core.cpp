/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Core.hpp"
#include "../../Config.hpp"
#include "../../Globals.hpp"
#include "../Api.hpp"
#include "../api/version.h"
#include "Config.hpp"
#include "Plugin.hpp"
#include <QDir>

using namespace M64P::Wrapper;

Core::Core(void)
{
}

Core::~Core(void)
{
}

#include <iostream>

void DebugCallback(void *Context, int level, const char *message)
{
   // std::cout << "DebugCallback: " << level << ": " << message << std::endl;
}

void StateCallback(void *Context2, m64p_core_param ParamChanged, int NewValue)
{
    //std::cout << "StateCallback: " << ParamChanged << ": " << NewValue << std::endl;
}

bool Core::Init(m64p_dynlib_handle handle)
{
    m64p_error ret;

    if (!M64P::Core.IsHooked())
    {
        this->error_Message = "M64P::Core is not hooked!";
        return false;
    }

    ret = M64P::Core.Startup(FRONTEND_API_VERSION, MUPEN_CONFIG_DIR, MUPEN_DATA_DIR, NULL, DebugCallback, NULL,
                             StateCallback);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::Init M64P::Core.Startup() Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    media_loader = {
        (void *)this,
        media_loader_get_gb_cart_rom,
        media_loader_get_gb_cart_ram,
        media_loader_get_dd_rom,
        media_loader_get_dd_disk
    };

    ret = M64P::Core.DoCommand(M64CMD_SET_MEDIA_LOADER, sizeof(media_loader), &media_loader);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::Init M64P::Core.DoCommand(M64CMD_SET_MEDIA_LOADER) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    this->handle = handle;

    return true;
}

bool Core::HasPluginConfig(PluginType type)
{
    return this->plugin_Get(type)->HasConfig();
}

bool Core::OpenPluginConfig(PluginType type)
{
    bool ret, paused;

    paused = this->emulation_IsPaused();

    if (!paused)
        this->PauseEmulation();

    ret = this->plugin_Get(type)->OpenConfig();

    if (!paused)
        this->ResumeEmulation();

    return ret;
}

bool Core::TakeScreenshot(void)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_TAKE_NEXT_SCREENSHOT, 0, NULL);

    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::TakeScreenshot M64P::Core.DoCommand(M64CMD_TAKE_NEXT_SCREENSHOT) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::EnableSpeedLimiter(void)
{
    return this->emulation_SpeedLimited(true);
}

bool Core::DisableSpeedLimiter(void)
{
    return this->emulation_SpeedLimited(false);
}

QList<Plugin_t> Core::GetPlugins(PluginType type)
{
    QList<Plugin_t> plugins;
    QString dir;

    switch (type)
    {
    default:
    case PluginType::Gfx:
        dir = MUPEN_DIR_GFX;
        break;
    case PluginType::Rsp:
        dir = MUPEN_DIR_RSP;
        break;
    case PluginType::Audio:
        dir = MUPEN_DIR_AUDIO;
        break;
    case PluginType::Input:
        dir = MUPEN_DIR_INPUT;
        break;
    }

    QDir qDir(dir);
    QStringList filter;

    filter << "*." SO_EXT;

    QFileInfoList fileList = qDir.entryInfoList(filter);

    Plugin p;
    Plugin_t pInfo;

    for (const QFileInfo &info : fileList)
    {
        if (p.Init(info.filePath(), this->handle))
        {
            pInfo = p.GetPlugin_t();

            if (pInfo.Type == type)
                plugins.append(pInfo);
        }
    }

    return plugins;
}

M64P::Wrapper::Plugin *Core::plugin_Get(PluginType type)
{
    switch (type)
    {
    case PluginType::Gfx:
        return &this->plugin_Gfx;
    case PluginType::Rsp:
        return &this->plugin_Rsp;
    case PluginType::Audio:
        return &this->plugin_Audio;
    case PluginType::Input:
        return &this->plugin_Input;
    default:
        return nullptr;
    }
}

char *Core::media_loader_get_gb_cart_rom(void *_this, int)
{
    return NULL;
}

char *Core::media_loader_get_gb_cart_ram(void *_this, int)
{
    return NULL;
}

char *Core::media_loader_get_dd_rom(void *_this)
{
    QString file = g_Settings.GetStringValue(SettingsID::Core_64DD_RomFile);

    if (file.isEmpty())
        return NULL;

    return strdup(file.toStdString().c_str());;
}

char *Core::media_loader_get_dd_disk(void *_this)
{
    QString file = ((Core*)_this)->rom_Combo_Disk;

    if (file.isEmpty())
        return NULL;

    return strdup(file.toStdString().c_str());
}

bool Core::plugin_Attach(Plugin *p)
{
    m64p_error ret;

    ret = M64P::Core.AttachPlugin(p->GetType(), p->GetHandle());

    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::plugin_Attach ( " + p->GetPlugin_t().Name + " ) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::plugins_Attach(void)
{
    return this->plugin_Attach(&this->plugin_Gfx) && this->plugin_Attach(&this->plugin_Audio) &&
           this->plugin_Attach(&this->plugin_Input) && this->plugin_Attach(&this->plugin_Rsp);
}

bool Core::plugins_Detach(void)
{
    M64P::Core.DetachPlugin(M64PLUGIN_GFX);
    M64P::Core.DetachPlugin(M64PLUGIN_AUDIO);
    M64P::Core.DetachPlugin(M64PLUGIN_INPUT);
    M64P::Core.DetachPlugin(M64PLUGIN_RSP);
    return true;
}

bool Core::plugin_LoadTodo(void)
{
    if (!this->plugin_Todo.isEmpty())
    {
        for (const Plugin_t &p : this->plugin_Todo)
        {
            if (!this->SetPlugin(p))
                return false;
        }

        this->plugin_Todo.clear();
    }

    return true;
}

bool Core::SetPlugin(Plugin_t plugin)
{
    bool ret;

    // don't apply plugins when emulation is running
    if (this->IsEmulationRunning() || this->isEmulationPaused())
    {
        plugin_Todo.append(plugin);
        return true;
    }

    Plugin *p = this->plugin_Get(plugin.Type);

    if (p->HasInit())
    {
        ret = p->Shutdown();
        if (!ret)
        {
            this->error_Message = "Core::SetPlugin p->Shutdown() Failed: ";
            this->error_Message += p->GetLastError();
            return false;
        }
    }

    ret = p->Init(plugin.FileName, this->handle);
    if (!ret)
    {
        this->error_Message = "Core::SetPlugin p->Init() Failed: ";
        this->error_Message += p->GetLastError();
        return false;
    }

    ret = p->Startup();
    if (!ret)
    {
        this->error_Message = "Core::SetPlugin p->Startup() Failed: ";
        this->error_Message += p->GetLastError();
        return false;
    }

    return true;
}

bool Core::GetCurrentPlugin(PluginType type, Plugin_t *plugin_t)
{
    Plugin *p = this->plugin_Get(type);

    if (!p->HasInit())
        return false;

    *plugin_t = p->GetPlugin_t();

    return true;
}

bool Core::GetRomInfo(RomInfo_t *info)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_ROM_GET_HEADER, sizeof(m64p_rom_header), &info->Header);
    if (ret != M64ERR_SUCCESS)
    {
        this->rom_Close();
        this->error_Message = "Core::GetRomInfo: M64P::Core.DoCommand(M64CMD_ROM_GET_HEADER) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    ret = M64P::Core.DoCommand(M64CMD_ROM_GET_SETTINGS, sizeof(m64p_rom_settings), &info->Settings);
    if (ret != M64ERR_SUCCESS)
    {
        this->rom_Close();
        this->error_Message = "Core::GetRomInfo: M64P::Core.DoCommand(M64CMD_ROM_GET_SETTINGS) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

bool Core::GetRomInfo(QString file, RomInfo_t *info, bool overlay = true)
{
    m64p_error ret;

    if (!this->rom_Open(file, overlay))
        return false;

    ret = M64P::Core.DoCommand(M64CMD_ROM_GET_HEADER, sizeof(m64p_rom_header), &info->Header);
    if (ret != M64ERR_SUCCESS)
    {
        this->rom_Close();
        this->error_Message = "Core::GetRomInfo: M64P::Core.DoCommand(M64CMD_ROM_GET_HEADER) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    ret = M64P::Core.DoCommand(M64CMD_ROM_GET_SETTINGS, sizeof(m64p_rom_settings), &info->Settings);
    if (ret != M64ERR_SUCCESS)
    {
        this->rom_Close();
        this->error_Message = "Core::GetRomInfo: M64P::Core.DoCommand(M64CMD_ROM_GET_SETTINGS) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    info->FileName = file;

    if (!this->rom_Close())
        return false;

    return true;
}

bool Core::GetDefaultRomInfo(RomInfo_t *romInfo)
{
    if (!this->emulation_IsRunning() && !this->emulation_IsPaused())
        return false;

    *romInfo = this->rom_Info;

    return true;
}

bool Core::LaunchEmulation(QString file)
{
    m64p_error ret;

    if (!this->plugin_LoadTodo())
        return false;

    if (!this->GetRomInfo(file, &this->rom_Info, false))
        return false;

    if (!this->rom_Open(file, true))
        return false;

    if (!this->rom_ApplyPluginOverlay())
        return false;

    if (!this->core_ApplyOverlay())
        return false;

    if (!this->plugins_Attach())
        return false;

    ret = M64P::Core.DoCommand(M64CMD_EXECUTE, 0, NULL);

    this->plugins_Detach();

    if (ret != M64ERR_SUCCESS)
    {
        this->rom_Close();
        this->error_Message = "Core::LaunchEmulation M64P::Core.DoCommand(M64CMD_EXECUTE) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    if (!this->rom_Close())
        return false;

    if (!this->plugin_LoadTodo())
        return false;

    return true;
}

bool Core::LaunchEmulation(QString cartRom, QString diskRom)
{
    this->rom_Combo_Disk = diskRom;

    return this->LaunchEmulation(cartRom);
}

bool Core::StopEmulation(void)
{
    m64p_error ret;

    if (!this->emulation_IsRunning())
        return true;

    ret = M64P::Core.DoCommand(M64CMD_STOP, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::StopEmulation M64P::Core.DoCommand(M64CMD_STOP) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

bool Core::PauseEmulation(void)
{
    m64p_error ret;

    if (!this->emulation_IsRunning())
        return false;

    ret = M64P::Core.DoCommand(M64CMD_PAUSE, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::PauseEmulation M64P::Core.DoCommand(M64CMD_PAUSE) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::ResumeEmulation(void)
{
    m64p_error ret;

    if (!this->emulation_IsPaused())
        return false;

    ret = M64P::Core.DoCommand(M64CMD_RESUME, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::ResumeEmulation M64P::Core.DoCommand(M64CMD_RESUME) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::IsEmulationRunning(void)
{
    return emulation_IsRunning();
}

bool Core::isEmulationPaused(void)
{
    return emulation_IsPaused();
}

bool Core::PressGameSharkButton(void)
{
    m64p_error ret;
    int value = 1;

    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_INPUT_GAMESHARK, &value);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::PressGameSharkButton Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    value = 0;
    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_INPUT_GAMESHARK, &value);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::PressGameSharkButton Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

bool Core::SetSaveSlot(int slot)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_STATE_SET_SLOT, slot, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SetSaveSlot M64P::Core.DoCommand(M64CMD_STATE_SET_SLOT) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::SaveStateAsFile(QString file)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_STATE_SAVE, 1, (void *)file.toStdString().c_str());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SaveStateAsFile: M64P::Core.DoCommand(M64CMD_STATE_SAVE) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::LoadStateFromFile(QString file)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_STATE_LOAD, 0, (void *)file.toStdString().c_str());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SaveStateAsFile: M64P::Core.DoCommand(M64CMD_STATE_LOAD) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::SaveState(void)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_STATE_SAVE, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SaveState: M64P::Core.DoCommand(M64CMD_STATE_SAVE) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::LoadState(void)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_STATE_LOAD, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::LoadState: M64P::Core.DoCommand(M64CMD_STATE_LOAD) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::SetKeyDown(int key, int mod)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_SEND_SDL_KEYDOWN, (mod << 16) + key, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SetKeyDown: M64P::Core.DoCommand(M64CMD_SEND_SDL_KEYDOWN) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::SetKeyUp(int key, int mod)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_SEND_SDL_KEYUP, (mod << 16) + key, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SetKeyUp: M64P::Core.DoCommand(M64CMD_SEND_SDL_KEYUP) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::SetVideoSize(int width, int height, bool force = false)
{
    int size = (width << 16) + height;
    int currSize = 0;
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_SIZE, &currSize);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SetVideoSize M64P::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    if (!force && currSize == size)
        return true;

    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_VIDEO_SIZE, &size);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::SetVideoSize M64P::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

bool Core::ToggleFullscreen(void)
{
    m64p_video_mode mode = M64VIDEO_NONE;
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_MODE, &mode);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::ToggleFullscreen M64P::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    if (mode == M64VIDEO_FULLSCREEN)
        mode = M64VIDEO_WINDOWED;
    else
        mode = M64VIDEO_FULLSCREEN;

    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_VIDEO_MODE, &mode);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::ToggleFullscreen M64P::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

bool Core::ResetEmulation(bool hard)
{
    m64p_error ret;

    if (!this->emulation_IsPaused() && !this->emulation_IsRunning())
        return false;

    ret = M64P::Core.DoCommand(M64CMD_RESET, hard ? 1 : 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::ResetEmulation M64P::Core.DoCommand(M64CMD_RESET) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

QString Core::GetLastError(void)
{
    return this->error_Message;
}

bool Core::rom_Open(QString file, bool overlay = true)
{
    m64p_error ret;
    QByteArray buffer;
    QFile qFile(file);

    if (!qFile.open(QIODevice::ReadOnly))
    {
        this->error_Message = "Core::rom_Open: QFile::open Failed";
        return false;
    }

    buffer = qFile.readAll();

    ret = M64P::Core.DoCommand(M64CMD_ROM_OPEN, buffer.size(), buffer.data());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::rom_Open: M64P::Core.DoCommand(M64CMD_ROM_OPEN) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    buffer.clear();
    qFile.close();

    if (ret != M64ERR_SUCCESS)
        return false;

    if (overlay)
        return this->rom_ApplyOverlay();

    return true;
}

bool Core::rom_ApplyPluginOverlay(void)
{
    bool ret;
    m64p_error ret2;
    RomInfo_t info = {0};
    QString section, value;

    ret = this->GetRomInfo(&info);
    if (!ret)
        return false;

    section = info.Settings.MD5;

    ret = g_MupenApi.Config.SectionExists(section);
    if (!ret)
        return true;

    SettingsID settingIdArray[] = {SettingsID::Game_GFX_Plugin, SettingsID::Game_AUDIO_Plugin,
                                   SettingsID::Game_INPUT_Plugin, SettingsID::Game_RSP_Plugin};

    for (int i = 0; i < 4; i++)
        this->plugin_Todo.append(g_Plugins.GetCurrentPlugin((PluginType)i));

    for (int i = 0; i < 4; i++)
    {
        value = g_Settings.GetStringValue(settingIdArray[i], section);
        if (!value.isEmpty())
        {
            Plugin_t plugin = {.FileName = value};
            if (!this->SetPlugin(plugin))
                return false;
        }
    }

    return true;
}

bool Core::rom_HasPluginOverlay(QString file)
{
    bool ret;
    RomInfo_t info = {0};
    QString section;

    if (!this->rom_Open(file))
        return false;

    ret = this->GetRomInfo(&info);
    if (!ret)
        return false;

    this->rom_Close();

    section = info.Settings.MD5;

    ret = g_MupenApi.Config.SectionExists(section);
    if (!ret)
        return false;

    SettingsID settingIdArray[] = {SettingsID::Game_GFX_Plugin, SettingsID::Game_AUDIO_Plugin,
                                   SettingsID::Game_INPUT_Plugin, SettingsID::Game_RSP_Plugin};

    for (int i = 0; i < 4; i++)
    {
        if (!g_Settings.GetStringValue(settingIdArray[i], section).isEmpty())
            return true;
    }

    return false;
}

#include <iostream>
bool Core::rom_ApplyOverlay(void)
{
    bool ret;
    m64p_error ret2;
    RomInfo_t info = {0};
    QString section;

    ret = this->GetRomInfo(&info);
    if (!ret)
        return false;

    section = info.Settings.MD5;

    ret = g_MupenApi.Config.SectionExists(section);
    if (!ret)
    {
        std::cout << "section: " << info.Settings.MD5 << " doesn't exist!" << std::endl;
        return true;
    }

    info.Settings.disableextramem = g_Settings.GetBoolValue(SettingsID::Game_DisableExtraMem, section);
    info.Settings.savetype = g_Settings.GetIntValue(SettingsID::Game_SaveType, section);
    info.Settings.countperop = g_Settings.GetIntValue(SettingsID::Game_CountPerOp, section);
    info.Settings.sidmaduration = g_Settings.GetIntValue(SettingsID::Game_SiDmaDuration, section);

    std::cout << "info.Settings.goodname: " << info.Settings.goodname << std::endl;
    std::cout << "info.Settings.DisableExtraMem: " << (int)info.Settings.disableextramem << std::endl;
    std::cout << "info.Settings.savetype: " << (int)info.Settings.savetype << std::endl;
    std::cout << "info.Settings.CountPerOp: " << info.Settings.countperop << std::endl;
    std::cout << "info.Settings.SiDmaDuration: " << info.Settings.sidmaduration << std::endl;

    ret2 = M64P::Core.DoCommand(M64CMD_ROM_SET_SETTINGS, sizeof(info.Settings), &info.Settings);
    if (ret2 != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::rom_ApplyOverlay: M64P::Core.DoCommand(M64CMD_ROM_SET_SETTINGS) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret2);
    }

    return ret2 == M64ERR_SUCCESS;
}

bool Core::core_ApplyOverlay(void)
{
    bool ret;
    m64p_error ret2;
    RomInfo_t info = {0};
    QString section;

    // copy settings from g_Settings to Core section
    g_MupenApi.Config.SetOption("Core", "RandomizeInterrupt",
                                g_Settings.GetBoolValue(SettingsID::Core_RandomizeInterrupt));
    g_MupenApi.Config.SetOption("Core", "R4300Emulator", g_Settings.GetIntValue(SettingsID::Core_CPU_Emulator));
    g_MupenApi.Config.SetOption("Core", "DisableExtraMem", g_Settings.GetBoolValue(SettingsID::Core_DisableExtraMem));
    g_MupenApi.Config.SetOption("Core", "EnableDebugger", g_Settings.GetBoolValue(SettingsID::Core_EnableDebugger));
    g_MupenApi.Config.SetOption("Core", "CountPerOp", g_Settings.GetIntValue(SettingsID::Core_CountPerOp));
    g_MupenApi.Config.SetOption("Core", "SiDmaDuration", g_Settings.GetIntValue(SettingsID::Core_SiDmaDuration));

    ret = this->GetRomInfo(&info);
    if (!ret)
        return false;

    section = info.Settings.MD5;

    ret = g_MupenApi.Config.SectionExists(section);
    if (!ret)
        return true;

    ret = g_Settings.GetBoolValue(SettingsID::Game_OverrideCoreSettings, section);
    if (!ret)
        return true;

    g_MupenApi.Config.SetOption("Core", "RandomizeInterrupt",
                                g_Settings.GetBoolValue(SettingsID::Game_RandomizeInterrupt, section));
    g_MupenApi.Config.SetOption("Core", "R4300Emulator",
                                g_Settings.GetIntValue(SettingsID::Game_CPU_Emulator, section));
    return true;
}

bool Core::rom_Close(void)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_ROM_CLOSE, 0, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::rom_Close: M64P::Core.DoCommand(M64CMD_ROM_CLOSE) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::emulation_QueryState(m64p_emu_state *state)
{
    m64p_error ret;

    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, state);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::emulation_QueryState: M64P::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::emulation_IsRunning(void)
{
    m64p_emu_state state;

    if (!this->emulation_QueryState(&state))
        return false;

    return state == M64EMU_RUNNING;
}

bool Core::emulation_IsPaused(void)
{
    m64p_emu_state state;

    if (!this->emulation_QueryState(&state))
        return false;

    return state == M64EMU_PAUSED;
}

bool Core::emulation_SpeedLimited(bool enabled)
{
    m64p_error ret;

    int value = enabled ? 1 : 0;

    ret = M64P::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_SPEED_LIMITER, &value);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::emulation_SpeedLimited: M64P::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}
