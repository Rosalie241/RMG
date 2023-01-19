/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "VidExt.hpp"

#include <RMG-Core/VidExt.hpp>
#include <RMG-Core/m64p/Api.hpp>
#include "OnScreenDisplay.hpp"

#include <QApplication>
#include <QOpenGLContext>
#include <QThread>
#include <QScreen>

//
// Local Variables
//

static Thread::EmulationThread* l_EmuThread          = nullptr;
static UserInterface::MainWindow* l_MainWindow       = nullptr;
static UserInterface::Widget::OGLWidget* l_OGLWidget = nullptr;
static QThread* l_RenderThread                       = nullptr;
static bool l_VidExtInitialized                      = false;
static bool l_OsdInitialized                         = false;
static QSurfaceFormat l_SurfaceFormat;

//
// VidExt Functions
//

static void VidExt_OglSetup(void)
{
    l_EmuThread->on_VidExt_SetupOGL(l_SurfaceFormat, QThread::currentThread());

    while (!l_OGLWidget->isVisible() ||
           !l_OGLWidget->GetContext()->isValid())
    {
        continue;
    }

    l_OGLWidget->GetContext()->makeCurrent(l_OGLWidget);
    l_VidExtInitialized = true;
}

static m64p_error VidExt_Init(void)
{
    l_RenderThread = QThread::currentThread();

    l_SurfaceFormat = QSurfaceFormat::defaultFormat();
    l_SurfaceFormat.setOption(QSurfaceFormat::DeprecatedFunctions, 1);
    l_SurfaceFormat.setDepthBufferSize(24);
    l_SurfaceFormat.setProfile(QSurfaceFormat::CompatibilityProfile);
    l_SurfaceFormat.setMajorVersion(3);
    l_SurfaceFormat.setMinorVersion(0);
    l_SurfaceFormat.setSwapInterval(0);

    l_EmuThread->on_VidExt_Init();

    return M64ERR_SUCCESS;
}

static m64p_error VidExt_Quit(void)
{
    l_OGLWidget->MoveContextToThread(QApplication::instance()->thread());
    l_EmuThread->on_VidExt_Quit();
    OnScreenDisplayShutdown();

    l_VidExtInitialized = false;
    l_OsdInitialized    = false;

    return M64ERR_SUCCESS;
}

static m64p_error VidExt_ListModes(m64p_2d_size *SizeArray, int *NumSizes)
{
    return M64ERR_UNSUPPORTED;
}

static m64p_error VidExt_ListRates(m64p_2d_size Size, int *NumRates, int *Rates)
{
    return M64ERR_UNSUPPORTED;
}

static m64p_error VidExt_SetMode(int Width, int Height, int BitsPerPixel, int ScreenMode, int Flags)
{
    if (!l_VidExtInitialized)
    {
        VidExt_OglSetup();
    }

    // try to initialize the OSD
    // when opengl 3 is used
    if (l_SurfaceFormat.majorVersion() == 3 &&
        !l_OsdInitialized)
    {
        if (!OnScreenDisplayInit())
        {
            return M64ERR_SYSTEM_FAIL;
        }

        OnScreenDisplayLoadSettings();
        l_OsdInitialized = true;
    }

    switch (ScreenMode)
    {
        default:
        case M64VIDEO_NONE:
            return M64ERR_INPUT_INVALID;
        case M64VIDEO_WINDOWED:
            l_EmuThread->on_VidExt_SetWindowedMode(Width, Height, BitsPerPixel, Flags);
            break;
        case M64VIDEO_FULLSCREEN:
            l_EmuThread->on_VidExt_SetFullscreenMode(Width, Height, BitsPerPixel, Flags);
            break;
    }

    OnScreenDisplaySetDisplaySize(Width, Height);
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_SetModeWithRate(int Width, int Height, int RefreshRate, int BitsPerPixel, int ScreenMode, int Flags)
{
    return VidExt_SetMode(Width, Height, BitsPerPixel, ScreenMode, Flags);
}

static m64p_function VidExt_GLGetProc(const char *Proc)
{
    return l_OGLWidget->GetContext()->getProcAddress(Proc);
}

static m64p_error VidExt_GLSetAttr(m64p_GLattr Attr, int Value)
{
    switch (Attr)
    {
    case M64P_GL_DOUBLEBUFFER:
        if (Value == 1)
            l_SurfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        else if (Value == 0)
            l_SurfaceFormat.setSwapBehavior(QSurfaceFormat::SingleBuffer);
        break;
    case M64P_GL_BUFFER_SIZE:
        break;
    case M64P_GL_DEPTH_SIZE:
        l_SurfaceFormat.setDepthBufferSize(Value);
        break;
    case M64P_GL_RED_SIZE:
        l_SurfaceFormat.setRedBufferSize(Value);
        break;
    case M64P_GL_GREEN_SIZE:
        l_SurfaceFormat.setGreenBufferSize(Value);
        break;
    case M64P_GL_BLUE_SIZE:
        l_SurfaceFormat.setBlueBufferSize(Value);
        break;
    case M64P_GL_ALPHA_SIZE:
        l_SurfaceFormat.setAlphaBufferSize(Value);
        break;
    case M64P_GL_SWAP_CONTROL:
        l_SurfaceFormat.setSwapInterval(Value);
        break;
    case M64P_GL_MULTISAMPLEBUFFERS:
        break;
    case M64P_GL_MULTISAMPLESAMPLES:
        l_SurfaceFormat.setSamples(Value);
        break;
    case M64P_GL_CONTEXT_MAJOR_VERSION:
        l_SurfaceFormat.setMajorVersion(Value);
        break;
    case M64P_GL_CONTEXT_MINOR_VERSION:
        l_SurfaceFormat.setMinorVersion(Value);
        break;
    case M64P_GL_CONTEXT_PROFILE_MASK:
        switch (Value)
        {
        case M64P_GL_CONTEXT_PROFILE_CORE:
            l_SurfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
            break;
        case M64P_GL_CONTEXT_PROFILE_COMPATIBILITY:
            l_SurfaceFormat.setProfile(QSurfaceFormat::CompatibilityProfile);
            break;
        case M64P_GL_CONTEXT_PROFILE_ES:
            l_SurfaceFormat.setRenderableType(QSurfaceFormat::OpenGLES);
            break;
        }

        break;
    }

    return M64ERR_SUCCESS;
}

static m64p_error VidExt_GLGetAttr(m64p_GLattr Attr, int *pValue)
{
    QSurfaceFormat::SwapBehavior SB = l_SurfaceFormat.swapBehavior();
    switch (Attr)
    {
    case M64P_GL_DOUBLEBUFFER:
        if (SB == QSurfaceFormat::SingleBuffer)
            *pValue = 0;
        else
            *pValue = 1;
        break;
    case M64P_GL_BUFFER_SIZE:
        *pValue =
            l_SurfaceFormat.alphaBufferSize() + l_SurfaceFormat.redBufferSize() + l_SurfaceFormat.greenBufferSize() + l_SurfaceFormat.blueBufferSize();
        break;
    case M64P_GL_DEPTH_SIZE:
        *pValue = l_SurfaceFormat.depthBufferSize();
        break;
    case M64P_GL_RED_SIZE:
        *pValue = l_SurfaceFormat.redBufferSize();
        break;
    case M64P_GL_GREEN_SIZE:
        *pValue = l_SurfaceFormat.greenBufferSize();
        break;
    case M64P_GL_BLUE_SIZE:
        *pValue = l_SurfaceFormat.blueBufferSize();
        break;
    case M64P_GL_ALPHA_SIZE:
        *pValue = l_SurfaceFormat.alphaBufferSize();
        break;
    case M64P_GL_SWAP_CONTROL:
        *pValue = l_SurfaceFormat.swapInterval();
        break;
    case M64P_GL_MULTISAMPLEBUFFERS:
        break;
    case M64P_GL_MULTISAMPLESAMPLES:
        *pValue = l_SurfaceFormat.samples();
        break;
    case M64P_GL_CONTEXT_MAJOR_VERSION:
        *pValue = l_SurfaceFormat.majorVersion();
        break;
    case M64P_GL_CONTEXT_MINOR_VERSION:
        *pValue = l_SurfaceFormat.minorVersion();
        break;
    case M64P_GL_CONTEXT_PROFILE_MASK:
        switch (l_SurfaceFormat.profile())
        {
        case QSurfaceFormat::CoreProfile:
            *pValue = M64P_GL_CONTEXT_PROFILE_CORE;
            break;
        case QSurfaceFormat::CompatibilityProfile:
            *pValue = M64P_GL_CONTEXT_PROFILE_COMPATIBILITY;
            break;
        case QSurfaceFormat::NoProfile:
            *pValue = M64P_GL_CONTEXT_PROFILE_COMPATIBILITY;
            break;
        }
        break;
    }
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_GLSwapBuf(void)
{
    if (l_RenderThread != QThread::currentThread())
    {
        return M64ERR_UNSUPPORTED;
    }

    OnScreenDisplayRender();

    l_OGLWidget->GetContext()->swapBuffers(l_OGLWidget);
    l_OGLWidget->GetContext()->makeCurrent(l_OGLWidget);

    return M64ERR_SUCCESS;
}

static m64p_error VidExt_SetCaption(const char *Title)
{
    l_EmuThread->on_VidExt_SetCaption(QString(Title));
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_ToggleFS(void)
{
    int videoMode = M64VIDEO_WINDOWED;

    if (m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_MODE, &videoMode) != M64ERR_SUCCESS)
    {
        return M64ERR_SYSTEM_FAIL;
    }

    if (QThread::currentThread() != l_RenderThread)
    {
        l_MainWindow->on_VidExt_ToggleFS((videoMode == M64VIDEO_WINDOWED));
    }
    else
    {
        l_EmuThread->on_VidExt_ToggleFS((videoMode == M64VIDEO_WINDOWED));
    }

    return M64ERR_SUCCESS;
}

static m64p_error VidExt_ResizeWindow(int Width, int Height)
{
    l_EmuThread->on_VidExt_ResizeWindow(Width, Height);
    OnScreenDisplaySetDisplaySize(Width, Height);
    return M64ERR_SUCCESS;
}

static uint32_t VidExt_GLGetDefaultFramebuffer(void)
{
    return l_OGLWidget->GetContext()->defaultFramebufferObject();
}

//
// Exported Functions
//

bool SetupVidExt(Thread::EmulationThread* emuThread, UserInterface::MainWindow* mainWindow, UserInterface::Widget::OGLWidget* oglWidget)
{
    l_EmuThread = emuThread;
    l_MainWindow = mainWindow;
    l_OGLWidget = oglWidget;

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

    return CoreSetupVidExt(vidext_funcs);
}


