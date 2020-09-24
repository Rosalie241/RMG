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

bool Api::Init(QString file)
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

    if (!this->core_VidExt_Override())
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

bool Api::core_VidExt_Override(void)
{
    m64p_error ret;
    m64p_video_extension_functions vidext_funcs;

    vidext_funcs.Functions = 14;
    vidext_funcs.VidExtFuncInit = &VidExt_Init;
    vidext_funcs.VidExtFuncQuit = &VidExt_Quit;
    vidext_funcs.VidExtFuncListModes = &VidExt_ListModes;
    vidext_funcs.VidExtFuncListRates = &VidExt_ListRates;
    vidext_funcs.VidExtFuncSetMode = &VidExt_SetMode;
    vidext_funcs.VidExtFuncSetModeWithRate = &VidExt_SetModeWithRate;
    vidext_funcs.VidExtFuncGLGetProc = &VidExt_GLGetProc;
    vidext_funcs.VidExtFuncGLSetAttr = &VidExt_GLSetAttr;
    vidext_funcs.VidExtFuncGLGetAttr = &VidExt_GLGetAttr;
    vidext_funcs.VidExtFuncGLSwapBuf = &VidExt_GLSwapBuf;
    vidext_funcs.VidExtFuncSetCaption = &VidExt_SetCaption;
    vidext_funcs.VidExtFuncToggleFS = &VidExt_ToggleFS;
    vidext_funcs.VidExtFuncResizeWindow = &VidExt_ResizeWindow;
    vidext_funcs.VidExtFuncGLGetDefaultFramebuffer = &VidExt_GLGetDefaultFramebuffer;

    ret = M64P::Core.OverrideVidExt(&vidext_funcs);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Api::core_VidExt_Override M64P::Core.OverrideVidExt Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}
