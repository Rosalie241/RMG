#ifndef M64P_CUSTOM_H
#define M64P_CUSTOM_H

#ifdef __cplusplus
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
typedef m64p_error (*ptr_PluginConfig)(void);
#if defined(M64P_PLUGIN_PROTOTYPES) || defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL PluginConfig(void);
#endif

/* PluginConfig2(int romConfig)
 *
 * This optional function opens a configuration GUI for the plugin
 * https://github.com/mupen64plus/mupen64plus-core/pull/774
 *
 * romConfig argument determines if it should open the ROM specific config
*/
typedef m64p_error (*ptr_PluginConfig2)(int);
#if defined(M64P_PLUGIN_PROTOTYPES) || defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL PluginConfig2(int);
#endif

/* PluginConfig2HasRomConfig(void)
 *
 * This optional function returns wether the plugin supports ROM specific config
 * using PluginConfig2
 * 
*/
typedef int (*ptr_PluginConfig2HasRomConfig)(void);
#if defined(M64P_PLUGIN_PROTOTYPES) || defined(M64P_CORE_PROTOTYPES)
EXPORT int CALL PluginConfig2HasRomConfig(void);

/* SetResetMousePositionCallback()
 *
 * This optional input plugin function sets a callback for when
 * the cursor needs to be reset to the initial position (every time GetKeys() is called)
 *
 */
typedef void (*ptr_ResetMousePositionCallback)(void);
typedef void (*ptr_SetResetMousePositionCallback)(ptr_ResetMousePositionCallback);
#if defined(M64P_PLUGIN_PROTOTYPES) || defined(M64P_CORE_PROTOTYPES)
EXPORT void CALL SetResetMousePositionCallback(ptr_ResetMousePositionCallback);
#endif

#ifdef __cplusplus
}
#endif

#endif // M64P_CUSTOM_H
