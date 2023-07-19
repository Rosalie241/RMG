#pragma once

#include "core/n64video.h"
#include "api/m64p_plugin.h"
#include "api/m64p_common.h"
#include "api/m64p_config.h"

#ifdef _WIN32
#define DLSYM(a, b) GetProcAddress(a, b)
#else
#include <dlfcn.h>
#define DLSYM(a, b) dlsym(a, b)
#endif

extern GFX_INFO gfx;
extern m64p_dynlib_handle CoreLibHandle;
extern m64p_handle configVideoAngrylionPlus;
extern void(*render_callback)(int);
extern void (*debug_callback)(void *, int, const char *);
extern void *debug_call_context;

extern ptr_ConfigOpenSection	  ConfigOpenSection;
extern ptr_ConfigSaveSection      ConfigSaveSection;
extern ptr_ConfigSetDefaultInt    ConfigSetDefaultInt;
extern ptr_ConfigSetDefaultBool   ConfigSetDefaultBool;
extern ptr_ConfigGetParamInt      ConfigGetParamInt;
extern ptr_ConfigGetParamBool     ConfigGetParamBool;
extern ptr_ConfigSetParameter     ConfigSetParameter;
extern ptr_PluginGetVersion       CoreGetVersion;

#define KEY_SCREEN_WIDTH "ScreenWidth"
#define KEY_SCREEN_HEIGHT "ScreenHeight"
#define KEY_PARALLEL "Parallel"
#define KEY_NUM_WORKERS "NumWorkers"
#define KEY_BUSY_LOOP "BusyLoop"

#define KEY_VI_MODE "ViMode"
#define KEY_VI_INTERP "ViInterpolation"
#define KEY_VI_WIDESCREEN "ViWidescreen"
#define KEY_VI_HIDE_OVERSCAN "ViHideOverscan"
#define KEY_VI_INTEGER_SCALING "ViIntegerScaling"

#define KEY_DP_COMPAT "DpCompat"
