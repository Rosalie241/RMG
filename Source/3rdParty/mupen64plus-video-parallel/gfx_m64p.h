#pragma once

#include "m64p_plugin.h"
#include "m64p_common.h"
#include "m64p_config.h"
#include "m64p_vidext.h"

#ifdef _WIN32
#define DLSYM(a, b) GetProcAddress(a, b)
#else
#include <dlfcn.h>
#define DLSYM(a, b) dlsym(a, b)
#endif

extern uint32_t rdram_size;
extern GFX_INFO gfx;
extern m64p_dynlib_handle CoreLibHandle;
extern m64p_handle configVideoParallel;
extern void (*render_callback)(int);
extern void (*debug_callback)(void *, int, const char *);
extern void *debug_call_context;

extern ptr_ConfigOpenSection ConfigOpenSection;
extern ptr_ConfigSaveSection ConfigSaveSection;
extern ptr_ConfigSetDefaultInt ConfigSetDefaultInt;
extern ptr_ConfigSetDefaultBool ConfigSetDefaultBool;
extern ptr_ConfigGetParamInt ConfigGetParamInt;
extern ptr_ConfigGetParamBool ConfigGetParamBool;
extern ptr_ConfigSetParameter ConfigSetParameter;


#define KEY_SCREEN_WIDTH "ScreenWidth"
#define KEY_SCREEN_HEIGHT "ScreenHeight"
#define KEY_UPSCALING "Upscaling"
#define KEY_WIDESCREEN "WidescreenStretch"
#define KEY_SSDITHER "SuperscaledDither"
#define KEY_SSREADBACKS "SuperscaledReads"
#define KEY_OVERSCANCROP "CropOverscan"
#define KEY_VERTICAL_STRETCH "VerticalStretch"
#define KEY_DIVOT "Divot"
#define KEY_GAMMADITHER "GammaDither"
#define KEY_AA "VIAA"
#define KEY_VIBILERP "VIBilerp"
#define KEY_VIDITHER "VIDither"
#define KEY_DOWNSCALE "DownScale"
#define KEY_NATIVETEXTRECT "NativeTextRECT"
#define KEY_NATIVETEXTLOD "NativeTextLOD"
#define KEY_DEINTERLACE "DeinterlaceMode"
#define KEY_SYNCHRONOUS "SynchronousRDP"
#define KEY_VSYNC "VSync"
