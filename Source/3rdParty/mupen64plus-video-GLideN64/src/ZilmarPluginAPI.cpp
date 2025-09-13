#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // OS_WINDOWS

#include "PluginAPI.h"
#include "N64.h"

extern "C" {

EXPORT void CALL RomOpen (void)
{
	RDRAMSize = 0;
	api().RomOpen();
}

#ifdef LEGACY_ZILMAR_SPEC
EXPORT void CALL CaptureScreen(char * Directory)
#else
EXPORT void CALL CaptureScreen(const char * Directory)
#endif
{
    api().CaptureScreen(Directory);
}

EXPORT void CALL CloseDLL (void)
{
	api().CloseDLL();
}

#ifdef LEGACY_ZILMAR_SPEC
EXPORT void CALL DllAbout(HWND hParent)
#else
EXPORT void CALL DllAbout(void * hParent)
#endif
{
    api().DllAbout(hParent);
}

#ifdef LEGACY_ZILMAR_SPEC
EXPORT void CALL DllConfig(HWND hParent)
#else
EXPORT void CALL DllConfig(void * hParent)
#endif
{
    api().DllConfig(hParent);
}

#ifdef LEGACY_ZILMAR_SPEC
EXPORT void CALL DllTest(HWND hParent)
#else
EXPORT void CALL DllTest(void * hParent)
#endif
{
    api().DllTest(hParent);
}

EXPORT void CALL DrawScreen (void)
{
	api().DrawScreen();
}

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
	api().GetDllInfo(PluginInfo);
}

EXPORT void CALL ReadScreen (void **dest, long *width, long *height)
{
	api().ReadScreen(dest, width, height);
}

EXPORT void CALL DllCrtFree(void* addr)
{
	free(addr);
}

void CALL mge_get_video_size(int32_t* width, int32_t* height)
{
	api().GetVideoSize(width, height);
}

EXPORT void CALL PluginLoaded(void)
{
    // No-op or initialization if needed
}

EXPORT void CALL DrawStatus(const char * lpString, int32_t RightAlign)
{
    api().DrawStatus(lpString, RightAlign);
}

}
