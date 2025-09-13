#include "GLideN64_Windows.h"
#include "../PluginAPI.h"
#include "../GLideN64.h"
#include "../GLideNUI/GLideNUI.h"
#include "../Config.h"
#include <DisplayWindow.h>
#include <Revision.h>

void PluginAPI::DllAbout(void* _hParent)
{
	HWND hParentWnd = (HWND)_hParent;
	Config_LoadConfig();
	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	api().FindPluginPath(strIniFolderPath);
	RunAbout(strIniFolderPath);
}

void PluginAPI::CaptureScreen(const char * const _Directory)
{
	dwnd().setCaptureScreen(_Directory);
}

void PluginAPI::DllConfig(void* _hParent)
{
  HWND hParentWnd = (HWND)_hParent;
	Config_DoConfig(nullptr);
}

void PluginAPI::GetDllInfo(PLUGIN_INFO * PluginInfo)
{
#ifdef LEGACY_ZILMAR_SPEC
	PluginInfo->Version = 0x103;
	PluginInfo->Type = PLUGIN_TYPE_GFX;
	sprintf(PluginInfo->Name, "%s", pluginNameWithRevision);
	PluginInfo->NormalMemory = FALSE;
	PluginInfo->MemoryBswaped = TRUE;
#else
	PluginInfo->Version = 0x105;
	PluginInfo->Type = PLUGIN_TYPE_VIDEO;
	sprintf(PluginInfo->Name, "%s", pluginNameWithRevision);
#endif
}

void PluginAPI::DrawStatus(const char * lpString, int32_t RightAlign)
{
    // Save current OSD position
    int prevPos = config.onScreenDisplay.pos;
    if (RightAlign)
        config.onScreenDisplay.pos = Config::posTopRight;
    else
        config.onScreenDisplay.pos = Config::posTopLeft;
    // Show the message for a short interval (e.g., 2 seconds)
    dwnd().getDrawer().showMessage(lpString, std::chrono::milliseconds(2000));
    // Restore previous OSD position
    config.onScreenDisplay.pos = prevPos;
}
