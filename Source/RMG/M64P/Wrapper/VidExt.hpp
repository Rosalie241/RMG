/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef VIDEXT_HPP
#define VIDEXT_HPP

#include "../../Thread/EmulationThread.hpp"
#include "../api/m64p_types.h"

m64p_error VidExt_Init(void);
m64p_error VidExt_Quit(void);
m64p_error VidExt_ListModes(m64p_2d_size *, int *);
m64p_error VidExt_ListRates(m64p_2d_size, int *, int *);
m64p_error VidExt_SetMode(int, int, int, int, int);
m64p_error VidExt_SetModeWithRate(int, int, int, int, int, int);
m64p_function VidExt_GLGetProc(const char *);
m64p_error VidExt_GLSetAttr(m64p_GLattr, int);
m64p_error VidExt_GLGetAttr(m64p_GLattr, int *);
m64p_error VidExt_GLSwapBuf(void);
m64p_error VidExt_SetCaption(const char *);
m64p_error VidExt_ToggleFS(void);
m64p_error VidExt_ResizeWindow(int, int);
uint32_t VidExt_GLGetDefaultFramebuffer(void);

#endif // VIDEXT_HPP