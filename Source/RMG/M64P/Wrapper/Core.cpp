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
#include "Plugin.hpp"
#include "../Api.hpp"
#include "../api/version.h"
#include "../../Config.hpp"

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
    //std::cout << level << ": " << message << std::endl;
}

void StateCallback(void *Context2, m64p_core_param ParamChanged, int NewValue)
{
    //std::cout << ParamChanged << ": " << NewValue << std::endl;
}

bool Core::Init(m64p_dynlib_handle handle)
{
    m64p_error ret;

    if (!M64P::Core.IsHooked())
    {
        this->error_Message = "M64P::Core is not hooked!";
        return false;
    }

    ret = M64P::Core.Startup(FRONTEND_API_VERSION, MUPEN_CONFIG_DIR, MUPEN_DATA_DIR, NULL, DebugCallback, NULL, StateCallback);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::Init M64P::Core.Startup() Failed: ";
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

#include <QDir>
QList<Plugin_t> Core::GetPlugins(PluginType type)
{
    QList<Plugin_t> plugins;
    QString dir;

    switch (type)
    {
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

    std::cout << dir.toStdString() << std::endl;

    QDir qDir(dir);
    QStringList filter;

    filter << "*." SO_EXT;

    QFileInfoList fileList = qDir.entryInfoList(filter);
    Plugin p;
    Plugin_t pInfo;

    for (const QFileInfo &info : fileList)
    {
        std::cout << info.absoluteFilePath().toStdString() << std::endl;
        if (p.Init(info.absoluteFilePath(), this->handle))
        {
            pInfo = p.GetPlugin_t();

            if (pInfo.Type != type)
                continue;

            plugins.append(pInfo);
        }
        else
        {
            std::cout << "Init failed??" << std::endl;
            std::cout << p.GetLastError().toStdString() << std::endl;
        }

        p.Shutdown();
    }

    for (const Plugin_t &plugin : plugins)
    {
        std::cout << "Name: " << plugin.Name.toStdString() << std::endl;
        std::cout << "FileName: " << plugin.FileName.toStdString() << std::endl;
        std::cout << "Version: " << plugin.Version << std::endl;
        this->SetPlugin(plugin);
    }

    return plugins;
}

bool Core::plugin_Attach(Plugin *p)
{
    m64p_error ret;

    ret = M64P::Core.AttachPlugin(p->GetType(), p->GetHandle());

    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::plugin_Attach Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Core::plugins_Attach(void)
{
    return this->plugin_Attach(&this->plugin_Gfx) &&
           this->plugin_Attach(&this->plugin_Audio) &&
           this->plugin_Attach(&this->plugin_Input) &&
           this->plugin_Attach(&this->plugin_Rsp);
}

bool Core::plugins_Detach(void)
{
    M64P::Core.DetachPlugin(M64PLUGIN_GFX);
    M64P::Core.DetachPlugin(M64PLUGIN_AUDIO);
    M64P::Core.DetachPlugin(M64PLUGIN_INPUT);
    M64P::Core.DetachPlugin(M64PLUGIN_RSP);
    return true;
}

bool Core::SetPlugin(Plugin_t plugin)
{
    bool ret;

    Plugin *p = this->plugin_Get(plugin.Type);

    ret = p->Init(plugin.FileName, this->handle);

    if (!ret)
    {
        std::cout << "Init failed?????" << std::endl;
        std::cout << p->GetLastError().toStdString() << std::endl;
        return false;
    }

    return true;
}

bool Core::GetRomInfo(QString file, RomInfo_t *info)
{
    m64p_error ret;
    bool inEmulation = this->emulation_IsRunning() || this->emulation_IsPaused();

    if (!inEmulation && !this->rom_Open(file))
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

    if (!inEmulation && !this->rom_Close())
        return false;

    return true;
}

bool Core::LaunchEmulation(QString file)
{
    m64p_error ret;

    if (!this->rom_Open(file))
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

    return true;
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

bool Core::rom_Open(QString file)
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

    if (buffer.data() == NULL)
        std::cout << "WARNING" << std::endl;

    ret = M64P::Core.DoCommand(M64CMD_ROM_OPEN, buffer.size(), buffer.data());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Core::rom_Open: M64P::Core.DoCommand(M64CMD_ROM_OPEN) Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    buffer.clear();
    qFile.close();

    return ret == M64ERR_SUCCESS;
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
