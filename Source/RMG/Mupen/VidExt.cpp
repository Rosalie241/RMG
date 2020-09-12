#include "VidExt.hpp"
#include <iostream>
#include <SDL.h>

static VidExtFuncs func;



m64p_error VidExtFuncInit(void)
{
    std::cout << "VidExtFuncInit" << std::endl;
    // func.VidExtFuncInit();
}

m64p_error VidExtFuncQuit(void)
{
    std::cout << "VidExtFuncQuit" << std::endl;
    // func.VidExtFuncQuit();
}

m64p_error VidExtFuncListModes(m64p_2d_size *SizeArray, int *NumSizes)
{
    SDL_Init(SDL_INIT_VIDEO);


    std::cout << "VidExtFuncListModes" << std::endl;

    int display = 0; //GetVideoDisplay();
    int modeCount = SDL_GetNumDisplayModes(display);
    SDL_DisplayMode displayMode;

    if (modeCount < 1)
    {
        std::cout << SDL_GetError() << std::endl;
        std::cout << "aa" << modeCount << std::endl;
        // DebugMessage(M64MSG_ERROR, "SDL_GetNumDisplayModes failed: %s", SDL_GetError());
        return M64ERR_SYSTEM_FAIL;
    }

    int displayModeCount = 0;
    int duplicateFound = 0;
    for (int i = 0; (i < modeCount) && (displayModeCount < *NumSizes); i++)
    {
        if (SDL_GetDisplayMode(display, i, &displayMode) < 0)
        {
            std::cout << SDL_GetError() << std::endl;
            // DebugMessage(M64MSG_ERROR, "SDL_GetDisplayMode failed: %s", SDL_GetError());
            return M64ERR_SYSTEM_FAIL;
        }

        // Check for duplicates
        duplicateFound = 0;
        for (int x = 0; x <= displayModeCount; x++)
        {
            if (SizeArray[x].uiHeight == displayMode.h &&
                SizeArray[x].uiWidth == displayMode.w)
            {
                duplicateFound = 1;
                break;
            }
        }

        // Skip duplicate
        if (duplicateFound == 1)
            continue;

        SizeArray[displayModeCount].uiHeight = displayMode.h;
        SizeArray[displayModeCount].uiWidth = displayMode.w;
        displayModeCount++;
    }

    std::cout << displayModeCount << std::endl;
    *NumSizes = displayModeCount;

    return M64ERR_SUCCESS;
}

m64p_error VidExtFuncListRates(m64p_2d_size Size, int *NumRates, int *Rates)
{
    std::cout << "VidExtFuncListRates" << std::endl;

    if (!SDL_WasInit(SDL_INIT_VIDEO))
        return M64ERR_NOT_INIT;

    int display = 0; //GetVideoDisplay();
    int modeCount = SDL_GetNumDisplayModes(display);
    SDL_DisplayMode displayMode;

    if (modeCount < 1)
    {
        // DebugMessage(M64MSG_ERROR, "SDL_GetNumDisplayModes failed: %s", SDL_GetError());
        return M64ERR_SYSTEM_FAIL;
    }

    int rateCount = 0;
    for (int i = 0; (i < modeCount) && (rateCount < *NumRates); i++)
    {
        if (SDL_GetDisplayMode(display, i, &displayMode) < 0)
        {
            // DebugMessage(M64MSG_ERROR, "SDL_GetDisplayMode failed: %s", SDL_GetError());
            return M64ERR_SYSTEM_FAIL;
        }

        /* skip when we're not at the right resolution */
        if (displayMode.w != Size.uiWidth ||
            displayMode.h != Size.uiHeight)
            continue;

        Rates[rateCount] = displayMode.refresh_rate;
        rateCount++;
    }

    *NumRates = rateCount;

    return M64ERR_SUCCESS;
}

m64p_error VidExtFuncSetMode(int, int, int, int, int)
{
    std::cout << "VidExtFuncSetMode" << std::endl;
}

m64p_error VidExtFuncSetModeWithRate(int, int, int, int, int, int)
{
    std::cout << "VidExtFuncSetModeWithRate" << std::endl;
}

m64p_function VidExtFuncGLGetProc(const char *)
{
    std::cout << "VidExtFuncGLGetProc" << std::endl;
}

m64p_error VidExtFuncGLSetAttr(m64p_GLattr, int)
{
    std::cout << "VidExtFuncGLSetAttr" << std::endl;
}

m64p_error VidExtFuncGLGetAttr(m64p_GLattr, int *)
{
    std::cout << "VidExtFuncGLGetAttr" << std::endl;
}

m64p_error VidExtFuncGLSwapBuf(void)
{
    std::cout << "VidExtFuncGLSwapBuf" << std::endl;
}

m64p_error VidExtFuncSetCaption(const char *)
{
    std::cout << "VidExtFuncSetCaption" << std::endl;
}

m64p_error VidExtFuncToggleFS(void)
{
    std::cout << "VidExtFuncToggleFS" << std::endl;
}

m64p_error VidExtFuncResizeWindow(int, int)
{
    std::cout << "VidExtFuncResizeWindow" << std::endl;
}

uint32_t VidExtFuncGLGetDefaultFramebuffer(void)
{
    std::cout << "VidExtFuncGLGetDefaultFramebuffer" << std::endl;
}

VidExt::VidExt(void)
{
    this->m64p_vidext_funcs.Functions = 14;
    this->m64p_vidext_funcs.VidExtFuncInit = &VidExtFuncInit;
    this->m64p_vidext_funcs.VidExtFuncQuit = &VidExtFuncQuit;
    this->m64p_vidext_funcs.VidExtFuncListModes = &VidExtFuncListModes;
    this->m64p_vidext_funcs.VidExtFuncListRates = &VidExtFuncListRates;
    this->m64p_vidext_funcs.VidExtFuncSetMode = &VidExtFuncSetMode;
    this->m64p_vidext_funcs.VidExtFuncSetModeWithRate = &VidExtFuncSetModeWithRate;
    this->m64p_vidext_funcs.VidExtFuncGLGetProc = &VidExtFuncGLGetProc;
    this->m64p_vidext_funcs.VidExtFuncGLSetAttr = &VidExtFuncGLSetAttr;
    this->m64p_vidext_funcs.VidExtFuncGLGetAttr = &VidExtFuncGLGetAttr;
    this->m64p_vidext_funcs.VidExtFuncGLSwapBuf = &VidExtFuncGLSwapBuf;
    this->m64p_vidext_funcs.VidExtFuncSetCaption = &VidExtFuncSetCaption;
    this->m64p_vidext_funcs.VidExtFuncToggleFS = &VidExtFuncToggleFS;
    this->m64p_vidext_funcs.VidExtFuncResizeWindow = &VidExtFuncResizeWindow;
    this->m64p_vidext_funcs.VidExtFuncGLGetDefaultFramebuffer = &VidExtFuncGLGetDefaultFramebuffer;
}

VidExt::~VidExt(void)
{
}

m64p_video_extension_functions *VidExt::VidExt_GetFuncs()
{
    return &this->m64p_vidext_funcs;
}

void VidExt::VidExt_Register(VidExtFuncs funcs)
{
    func = funcs;
}
