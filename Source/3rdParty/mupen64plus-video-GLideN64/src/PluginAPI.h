#ifndef COMMONPLUGINAPI_H
#define COMMONPLUGINAPI_H

#ifdef MUPENPLUSAPI
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "m64p_plugin.h"
#elif defined (LEGACY_ZILMAR_SPEC)
#include "windows/GLideN64_windows.h"
#include "windows/Project64-plugin-spec/1_3/Video.h"
#include "FrameBufferInfoAPI.h"
#else
#include "windows/GLideN64_windows.h"
#include "windows/Project64-plugin-spec/Video.h"
#include "FrameBufferInfoAPI.h"
//#define RSPTHREAD
#endif

#ifdef RSPTHREAD
#include <thread>
#include <condition_variable>
#endif

class APICommand;

class PluginAPI
{
public:
#ifdef RSPTHREAD
	~PluginAPI()
	{
		delete m_pRspThread;
		m_pRspThread = NULL;
	}
#endif

	// Common
	void MoveScreen(int /*_xpos*/, int /*_ypos*/) {}
	void ViStatusChanged() {}
	void ViWidthChanged() {}

	void ProcessDList();
	void ProcessRDPList();
	void RomClosed();
	int RomOpen();
	void ShowCFB();
	void UpdateScreen();
	int InitiateGFX(const GFX_INFO & _gfxInfo);
	void ChangeWindow();

	void FindPluginPath(wchar_t * _strPath);
	void GetUserDataPath(wchar_t * _strPath);
	void GetUserCachePath(wchar_t * _strPath);
#ifdef M64P_GLIDENUI
	void GetUserConfigPath(wchar_t * _strPath);
	void GetSharedDataPath(wchar_t * _strPath);
#endif // M64P_GLIDENUI
	bool isRomOpen() const { return m_bRomOpen; }

#ifndef MUPENPLUSAPI
	// Zilmar
	void DllTest(void* _hParent) {}
	void DrawScreen() {}
	void CloseDLL(void) {}

	void CaptureScreen(const char * const _Directory);
	void DllConfig(void* _hParent);
	void GetDllInfo (PLUGIN_INFO * PluginInfo);
	void ReadScreen(void **_dest, long *_width, long *_height);
	void GetVideoSize(int32_t* width, int32_t* height);

	void DllAbout(void* _hParent);
	void DrawStatus(const char * lpString, int32_t RightAlign);

	// FrameBufferInfo extension
	void FBWrite(unsigned int addr, unsigned int size);
	void FBWList(FrameBufferModifyEntry *plist, unsigned int size);
	void FBRead(unsigned int addr);
	void FBGetFrameBufferInfo(void *pinfo);
#else
	// MupenPlus
	void ResizeVideoOutput(int _Width, int _Height);
	void ReadScreen2(void * _dest, int * _width, int * _height, int _front);

	m64p_error PluginStartup(m64p_dynlib_handle _CoreLibHandle, void * Context, void (*DebugCallback)(void *, int, const char *));
#ifdef M64P_GLIDENUI
	m64p_error PluginConfig(void* parent);
#endif // M64P_GLIDENUI
	m64p_error PluginShutdown();
	m64p_error PluginGetVersion(
		m64p_plugin_type * _PluginType,
		int * _PluginVersion,
		int * _APIVersion,
		const char ** _PluginNamePtr,
		int * _Capabilities
	);
	void SetRenderingCallback(void (*callback)(int));

	// FrameBufferInfo extension
	void FBWrite(unsigned int addr, unsigned int size);
	void FBRead(unsigned int addr);
	void FBGetFrameBufferInfo(void *pinfo);
#endif

	static PluginAPI & get();

private:
	PluginAPI()
		: m_bRomOpen(false)
#ifdef RSPTHREAD
		, m_pRspThread(NULL)
		, m_pCommand(nullptr)
#endif
	{}
	PluginAPI(const PluginAPI &) = delete;

	void _initiateGFX(const GFX_INFO & _gfxInfo) const;

	bool m_bRomOpen;
#ifdef RSPTHREAD
	void _callAPICommand(APICommand & _command);
	std::mutex m_rspThreadMtx;
	std::mutex m_pluginThreadMtx;
	std::condition_variable_any m_rspThreadCv;
	std::condition_variable_any m_pluginThreadCv;
	std::thread * m_pRspThread;
	APICommand * m_pCommand;
#endif
};

inline PluginAPI & api()
{
	return PluginAPI::get();
}

#endif // COMMONPLUGINAPI_H
