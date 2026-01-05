/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - pif_sync_callback.c                                     *
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

#include "pif_sync_callback.h"
#include <stddef.h>

static pif_sync_callback_t g_pif_sync_callback = NULL;

void set_pif_sync_callback(pif_sync_callback_t callback)
{
    g_pif_sync_callback = callback;
}

void call_pif_sync_callback(struct pif* pif)
{
    if (g_pif_sync_callback != NULL) {
        g_pif_sync_callback(pif);
    }
}
