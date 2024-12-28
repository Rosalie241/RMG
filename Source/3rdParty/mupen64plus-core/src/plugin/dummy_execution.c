/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - dummy_execution.c                                       *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2019                                                    *
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

#include <stdlib.h>

#include "api/m64p_types.h"
#include "dummy_execution.h"
#include "plugin.h"

m64p_error dummyexecution_PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                           int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
    if (PluginType != NULL)
        *PluginType = M64PLUGIN_EXECUTION;

    if (PluginVersion != NULL)
        *PluginVersion = 0x00010000;

    if (APIVersion != NULL)
        *APIVersion = INPUT_API_VERSION;

    if (PluginNamePtr != NULL)
        *PluginNamePtr = "Mupen64Plus-NoExecution";

    if (Capabilities != NULL)
        *Capabilities = 0;

    return M64ERR_SUCCESS;
}

void dummyexecution_InitiateExecution()
{
}

void dummyexecution_StartExecution(EXECUTION_INFO ExecutionInfo)
{
}

int dummyexecution_RomOpen(void)
{
    return 1;
}

void dummyexecution_RomClosed(void)
{
}

void dummyexecution_Input(int Control, BUTTONS * Keys)
{
}

void dummyexecution_Frame(uint32_t frame)
{
}

void dummyexecution_Execute(uint32_t pc)
{
}

void dummyexecution_ExecuteDone(uint32_t pc)
{
}

void dummyexecution_Read8(uint8_t* value, uint32_t pc, uint32_t addr)
{
}

void dummyexecution_Read16(uint16_t* value, uint32_t pc, uint32_t addr)
{
}

void dummyexecution_Read32(uint32_t* value, uint32_t pc, uint32_t addr)
{
}

void dummyexecution_Read64(uint64_t* value, uint32_t pc, uint32_t addr)
{
}

void dummyexecution_Write8(uint8_t* value, uint32_t pc, uint32_t addr)
{
}

void dummyexecution_Write16(uint16_t* value, uint32_t pc, uint32_t addr)
{
}

void dummyexecution_Write32(uint32_t* value, uint32_t pc, uint32_t addr)
{
}

void dummyexecution_Write64(uint64_t* value, uint32_t pc, uint32_t addr)
{
}