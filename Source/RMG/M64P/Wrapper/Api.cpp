/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "Api.hpp"
#include "../Api.hpp"
#include "../Macros.hpp"
#include "../../Globals.hpp"
#include "VidExt.hpp"

using namespace M64P::Wrapper;

Api::Api(void)
{
}

Api::~Api(void)
{
    this->core_Handle_Close();
}

bool Api::Init(QString file, QOpenGLWidget* widget)
{
    this->error_Message = "Api::Init Failed: ";

    if (!this->core_Handle_Open(file))
        return false;

    if (!M64P::Core.Hook(this->core_Handle))
    {
        this->error_Message += M64P::Core.GetLastError();
        return false;
    }

    if (!M64P::Config.Hook(this->core_Handle))
    {
        this->error_Message += M64P::Config.GetLastError();
        return false;
    }

    if (!this->Core.Init(this->core_Handle))
    {
        this->error_Message += this->Core.GetLastError();
        return false;
    }

    if (!this->Config.Init())
    {
        this->error_Message += this->Config.GetLastError();
        return false;
    }

    if (!this->core_VidExt_Override(widget))
        return false;

    return true;
}

QString Api::GetLastError(void)
{
    return this->error_Message;
}

bool Api::core_Handle_Open(QString file)
{
    this->core_Handle_Close();

    this->core_Handle = DLOPEN(file.toStdString().c_str());
    if (this->core_Handle == NULL)
    {
        this->error_Message += DLGETERRSTR();
        return false;
    }

    this->core_Handle_Opened = true;

    return true;
}

void Api::core_Handle_Close(void)
{
    if (!this->core_Handle_Opened)
        return;

    DLCLOSE(this->core_Handle);
    this->core_Handle_Opened = false;
}

bool Api::core_VidExt_Override(QOpenGLWidget* widget)
{
    m64p_error ret;
    m64p_video_extension_functions Vidext_Funcs;

    Vidext_Funcs.Functions = 14;
    Vidext_Funcs.VidExtFuncInit = &VidExt_Init;
    Vidext_Funcs.VidExtFuncQuit = &VidExt_Quit;
    Vidext_Funcs.VidExtFuncListModes = &VidExt_ListModes;
    Vidext_Funcs.VidExtFuncListRates = &VidExt_ListRates;
    Vidext_Funcs.VidExtFuncSetMode = &VidExt_SetMode;
    Vidext_Funcs.VidExtFuncSetModeWithRate = &VidExt_SetModeWithRate;
    Vidext_Funcs.VidExtFuncGLGetProc = &VidExt_GLGetProc;
    Vidext_Funcs.VidExtFuncGLSetAttr = &VidExt_GLSetAttr;
    Vidext_Funcs.VidExtFuncGLGetAttr = &VidExt_GLGetAttr;
    Vidext_Funcs.VidExtFuncGLSwapBuf = &VidExt_GLSwapBuf;
    Vidext_Funcs.VidExtFuncSetCaption = &VidExt_SetCaption;
    Vidext_Funcs.VidExtFuncToggleFS = &VidExt_ToggleFS;
    Vidext_Funcs.VidExtFuncResizeWindow = &VidExt_ResizeWindow;
    Vidext_Funcs.VidExtFuncGLGetDefaultFramebuffer = &VidExt_GLGetDefaultFramebuffer;

    ret = M64P::Core.OverrideVidExt(&Vidext_Funcs);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Api::core_VidExt_Override M64P::Core.OverrideVidExt Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    VidExt_Setup(widget);

    return true;
}
