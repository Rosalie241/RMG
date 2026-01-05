/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - pif_sync_callback.h                                     *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2025 RMG Contributors                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef M64P_MAIN_PIF_SYNC_CALLBACK_H
#define M64P_MAIN_PIF_SYNC_CALLBACK_H

#include "api/m64p_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pif;

/* Callback function type for external PIF synchronization (e.g., Kaillera)
 * Called after netplay_update_input() to allow external sync implementations
 */
typedef void (*pif_sync_callback_t)(struct pif* pif);

/* Set PIF sync callback (call this from RMG-Core) */
EXPORT void CALL set_pif_sync_callback(pif_sync_callback_t callback);

/* Call the PIF sync callback if set (called from pif.c) */
void call_pif_sync_callback(struct pif* pif);

#ifdef __cplusplus
}
#endif

#endif
