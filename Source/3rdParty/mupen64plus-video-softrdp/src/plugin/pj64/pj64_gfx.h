#ifndef PJ64_GFX_H
#define PJ64_GFX_H

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#define PJ64_EXPORT __declspec(dllexport)
#define PJ64_CALL __cdecl
#else
typedef void *HWND;
typedef uint32_t DWORD;
typedef int BOOL;
#define PJ64_EXPORT
#define PJ64_CALL
#endif

#define PLUGIN_TYPE_GFX 2
#define PLUGIN_VERSION 0x0103

typedef struct PLUGIN_INFO {
    uint16_t Version;
    uint16_t Type;
    char Name[100];
    BOOL NormalMemory;
    BOOL MemoryBswaped;
} PLUGIN_INFO;

typedef struct GFX_INFO {
    HWND hWnd;
    HWND hStatusBar;
    BOOL MemoryBswaped;
    uint8_t *HEADER;
    uint8_t *RDRAM;
    uint8_t *DMEM;
    uint8_t *IMEM;
    DWORD *MI_INTR_REG;
    DWORD *DPC_START_REG;
    DWORD *DPC_END_REG;
    DWORD *DPC_CURRENT_REG;
    DWORD *DPC_STATUS_REG;
    DWORD *DPC_CLOCK_REG;
    DWORD *DPC_BUFBUSY_REG;
    DWORD *DPC_PIPEBUSY_REG;
    DWORD *DPC_TMEM_REG;
    DWORD *VI_STATUS_REG;
    DWORD *VI_ORIGIN_REG;
    DWORD *VI_WIDTH_REG;
    DWORD *VI_INTR_REG;
    DWORD *VI_V_CURRENT_LINE_REG;
    DWORD *VI_TIMING_REG;
    DWORD *VI_V_SYNC_REG;
    DWORD *VI_H_SYNC_REG;
    DWORD *VI_LEAP_REG;
    DWORD *VI_H_START_REG;
    DWORD *VI_V_START_REG;
    DWORD *VI_V_BURST_REG;
    DWORD *VI_X_SCALE_REG;
    DWORD *VI_Y_SCALE_REG;
    void (*CheckInterrupts)(void);
} GFX_INFO;

typedef struct FrameBufferModifyEntry {
    DWORD addr;
    DWORD val;
    DWORD size;
} FrameBufferModifyEntry;

PJ64_EXPORT void PJ64_CALL GetDllInfo(PLUGIN_INFO *plugin_info);
PJ64_EXPORT void PJ64_CALL CaptureScreen(char *directory);
PJ64_EXPORT void PJ64_CALL ChangeWindow(void);
PJ64_EXPORT BOOL PJ64_CALL InitiateGFX(GFX_INFO gfx_info);
PJ64_EXPORT void PJ64_CALL CloseDLL(void);
PJ64_EXPORT void PJ64_CALL RomOpen(void);
PJ64_EXPORT void PJ64_CALL RomClosed(void);
PJ64_EXPORT void PJ64_CALL ProcessDList(void);
PJ64_EXPORT void PJ64_CALL ProcessRDPList(void);
PJ64_EXPORT void PJ64_CALL DrawScreen(void);
PJ64_EXPORT void PJ64_CALL ReadScreen(void **dest, long *width, long *height);
PJ64_EXPORT void PJ64_CALL UpdateScreen(void);
PJ64_EXPORT void PJ64_CALL ViStatusChanged(void);
PJ64_EXPORT void PJ64_CALL ViWidthChanged(void);
PJ64_EXPORT void PJ64_CALL ShowCFB(void);
PJ64_EXPORT void PJ64_CALL MoveScreen(int xpos, int ypos);
PJ64_EXPORT void PJ64_CALL FBWrite(DWORD addr, DWORD size);
PJ64_EXPORT void PJ64_CALL FBWList(FrameBufferModifyEntry *plist, DWORD size);
PJ64_EXPORT void PJ64_CALL FBRead(DWORD addr);
PJ64_EXPORT void PJ64_CALL FBGetFrameBufferInfo(void *pinfo);
PJ64_EXPORT void PJ64_CALL DllAbout(HWND hwnd);
PJ64_EXPORT void PJ64_CALL DllConfig(HWND hwnd);
PJ64_EXPORT void PJ64_CALL DllTest(HWND hwnd);

#endif
