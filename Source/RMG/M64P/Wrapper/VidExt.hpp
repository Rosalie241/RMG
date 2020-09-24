#ifndef VIDEXT_HPP
#define VIDEXT_HPP

#include "../api/m64p_types.h"
#include "../../Thread/EmulationThread.hpp"
#include <QOpenGLWidget>

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