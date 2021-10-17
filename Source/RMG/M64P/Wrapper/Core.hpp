/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef M64P_WRAPPER_CORE_HPP
#define M64P_WRAPPER_CORE_HPP

#include "Plugin.hpp"
#include "Types.hpp"

#include <QList>
#include <QString>
#include <QObject>

namespace M64P
{
namespace Wrapper
{
class Core : public QObject
{
  Q_OBJECT
  public:
    Core(void);
    ~Core(void);

    bool Init(m64p_dynlib_handle);

    bool HasPluginConfig(PluginType);
    bool OpenPluginConfig(PluginType);

    QList<Plugin_t> GetPlugins(PluginType);
    bool SetPlugin(Plugin_t);
    bool GetCurrentPlugin(PluginType, Plugin_t *);

    bool GetRomInfo(QString, RomInfo_t *, bool);
    bool GetRomInfo(RomInfo_t *);
    bool GetDefaultRomInfo(RomInfo_t *);

    bool LaunchEmulation(QString);
    bool LaunchEmulation(QString, QString);
    bool StopEmulation(void);
    bool PauseEmulation(void);
    bool ResumeEmulation(void);
    bool ResetEmulation(bool);

    bool IsEmulationRunning(void);
    bool isEmulationPaused(void);

    bool TakeScreenshot(void);

    bool GetSpeedLimiterState(void);
    bool SetSpeedLimiter(bool);

    bool PressGameSharkButton(void);

    int GetSaveSlot(void);
    bool SetSaveSlot(int);

    bool SaveStateAsFile(QString);
    bool LoadStateFromFile(QString);

    bool SaveState(void);
    bool LoadState(void);

    bool SetKeyDown(int, int);
    bool SetKeyUp(int, int);

    bool SetVideoSize(int, int, bool);

    bool ToggleFullscreen(void);

    QString GetLastError(void);

  private:
    QString error_Message;

    m64p_media_loader media_loader;

    QString rom_Combo_Disk;

    static char* media_loader_get_gb_cart_rom(void*, int);
    static char* media_loader_get_gb_cart_ram(void*, int);
    static char* media_loader_get_dd_rom(void*);
    static char* media_loader_get_dd_disk(void*);

    m64p_dynlib_handle handle;

    QList<Plugin_t> plugin_Todo;
    M64P::Wrapper::Plugin plugin_Rsp;
    M64P::Wrapper::Plugin plugin_Gfx;
    M64P::Wrapper::Plugin plugin_Audio;
    M64P::Wrapper::Plugin plugin_Input;

    M64P::Wrapper::Plugin *plugin_Get(PluginType);
    bool plugin_Attach(Plugin *);
    bool plugins_Attach(void);
    bool plugins_Detach(void);
    bool plugin_LoadTodo(void);


    RomInfo_t rom_Info;

    bool rom_Open(QString, bool);
    bool rom_Unzip(QString, void**, int*);
    bool rom_ApplyPluginOverlay(void);
    bool rom_HasPluginOverlay(QString);
    bool rom_ApplyOverlay(void);
    bool rom_Close(void);

    static void core_DebugCallback(void *, int, const char *);
    static void core_StateCallback(void *, m64p_core_param, int);

    bool core_ApplyOverlay(void);

    bool emulation_QueryState(m64p_emu_state *);
    bool emulation_IsRunning(void);
    bool emulation_IsPaused(void);
  signals:
    void on_Core_DebugCallback(MessageType, QString);
};
} // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_CORE_HPP
