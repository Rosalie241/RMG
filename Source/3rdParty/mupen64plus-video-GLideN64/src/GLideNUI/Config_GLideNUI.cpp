#include "../N64.h"
#include "../Config.h"
#include "../RSP.h"
#include "../PluginAPI.h"
#include "../GLideNUI/GLideNUI.h"
#include <DisplayWindow.h>


Config config;

void Config_DoConfig(/*HWND hParent*/)
{
	if (ConfigOpen)
		return;

	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	wchar_t strSharedIniFolderPath[PLUGIN_PATH_SIZE];

#ifdef M64P_GLIDENUI
	api().FindPluginPath(strSharedIniFolderPath);
	api().GetUserConfigPath(strIniFolderPath);
#else
	api().FindPluginPath(strSharedIniFolderPath);
	api().FindPluginPath(strIniFolderPath);
#endif // M64P_GLIDENUI

	ConfigOpen = true;
	const u32 maxMsaa = dwnd().maxMSAALevel();
	const u32 maxAnisotropy = dwnd().maxAnisotropy();
	const bool bRestart = RunConfig(strIniFolderPath, strSharedIniFolderPath, api().isRomOpen() ? RSP.romname : nullptr, maxMsaa, maxAnisotropy);
	if (config.generalEmulation.enableCustomSettings != 0)
		LoadCustomRomSettings(strIniFolderPath, strSharedIniFolderPath, RSP.romname);
	config.validate();
	if (bRestart)
		dwnd().restart();
	ConfigOpen = false;
}

void Config_LoadConfig()
{
	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	wchar_t strSharedIniFolderPath[PLUGIN_PATH_SIZE];

#ifdef M64P_GLIDENUI
	api().FindPluginPath(strSharedIniFolderPath);
	api().GetUserConfigPath(strIniFolderPath);
#else
	api().FindPluginPath(strSharedIniFolderPath);
	api().FindPluginPath(strIniFolderPath);
#endif // M64P_GLIDENUI

	LoadConfig(strIniFolderPath, strSharedIniFolderPath);
	if (config.generalEmulation.enableCustomSettings != 0)
		LoadCustomRomSettings(strIniFolderPath, strSharedIniFolderPath, RSP.romname);
	config.validate();
}
