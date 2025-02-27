#ifndef M64P_CUSTOM_H
#define M64P_CUSTOM_H

#ifdef __cplusplus
#include <RMG-Core/RomSettings.hpp>
#include <RMG-Core/RomHeader.hpp>

extern "C" {
#endif

/* This file contains functions which break the mupen64plus spec */

#include "m64p_types.h"


/* PluginConfig()
 *
 * This optional function opens a configuration GUI for the plugin
 * https://github.com/mupen64plus/mupen64plus-core/pull/774
 * 
*/
typedef m64p_error (*ptr_PluginConfig)(void*);
#if defined(M64P_PLUGIN_PROTOTYPES) || defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL PluginConfig(void*);
#endif

#ifdef __cplusplus // we need C++ for the RMG-Core types


/* PluginConfig2(int romConfig)
 *
 * This optional function opens a configuration GUI for the plugin
 * https://github.com/mupen64plus/mupen64plus-core/pull/774
 *
 * romConfig argument determines if it should open the ROM specific config
*/
typedef m64p_error (*ptr_PluginConfig2)(void*, int, CoreRomHeader*, CoreRomSettings*);
#if defined(M64P_PLUGIN_PROTOTYPES) || defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL PluginConfig2(void*, int, CoreRomHeader*, CoreRomSettings*);
#endif

#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif // M64P_CUSTOM_H
