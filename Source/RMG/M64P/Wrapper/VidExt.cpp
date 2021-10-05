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
#include "Globals.hpp"

#include <QApplication>
#include <QOpenGLContext>
#include <QThread>
#include <QScreen>

static QSurfaceFormat format;
static QThread *renderThread;

static bool ogl_setup = false;
static void VidExt_OglSetup(void)
{
    g_EmuThread->on_VidExt_SetupOGL(format, QThread::currentThread());

    while (!g_OGLWidget->isValid())
        continue;

    g_OGLWidget->makeCurrent();

    ogl_setup = true;
}

m64p_error VidExt_Init(void)
{
    renderThread = QThread::currentThread();

    format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::DeprecatedFunctions, 1);
    format.setDepthBufferSize(24);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setMajorVersion(2);
    format.setMinorVersion(1);
    format.setSwapInterval(0);

    g_EmuThread->on_VidExt_Init();

    return M64ERR_SUCCESS;
}

m64p_error VidExt_Quit(void)
{
    g_OGLWidget->MoveToThread(QApplication::instance()->thread());
    g_EmuThread->on_VidExt_Quit();
    ogl_setup = false;

    return M64ERR_SUCCESS;
}

m64p_error VidExt_ListModes(m64p_2d_size *SizeArray, int *NumSizes)
{
    QSize size = QApplication::primaryScreen()->size();
    SizeArray[0].uiHeight = size.height();
    SizeArray[0].uiWidth = size.width();
    *NumSizes = 1;

    return M64ERR_SUCCESS;
}

m64p_error VidExt_ListRates(m64p_2d_size Size, int *NumRates, int *Rates)
{
    Rates[0] = QApplication::primaryScreen()->refreshRate();
    *NumRates = 1;

    return M64ERR_SUCCESS;
}

m64p_error VidExt_SetMode(int Width, int Height, int BitsPerPixel, int ScreenMode, int Flags)
{
    if (!ogl_setup)
    {
        VidExt_OglSetup();
    }

    g_EmuThread->on_VidExt_SetMode(Width, Height, BitsPerPixel, ScreenMode, Flags);
    return M64ERR_SUCCESS;
}

m64p_error VidExt_SetModeWithRate(int Width, int Height, int RefreshRate, int BitsPerPixel, int ScreenMode, int Flags)
{
    if (!ogl_setup)
    {
        VidExt_OglSetup();
    }

    switch (ScreenMode)
    {
        case M64VIDEO_NONE:
            return M64ERR_INPUT_INVALID;
        case M64VIDEO_WINDOWED:
            g_EmuThread->on_VidExt_SetWindowedModeWithRate(Width, Height, RefreshRate, BitsPerPixel, Flags);
            break;
        case M64VIDEO_FULLSCREEN:
            g_EmuThread->on_VidExt_SetFullscreenModeWithRate(Width, Height, RefreshRate, BitsPerPixel, Flags);
            break;
    }

    return M64ERR_SUCCESS;
}

m64p_function VidExt_GLGetProc(const char *Proc)
{
    return g_OGLWidget->context()->getProcAddress(Proc);
}

m64p_error VidExt_GLSetAttr(m64p_GLattr Attr, int Value)
{

    switch (Attr)
    {
    case M64P_GL_DOUBLEBUFFER:
        if (Value == 1)
            format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        else if (Value == 0)
            format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
        break;
    case M64P_GL_BUFFER_SIZE:
        break;
    case M64P_GL_DEPTH_SIZE:
        format.setDepthBufferSize(Value);
        break;
    case M64P_GL_RED_SIZE:
        format.setRedBufferSize(Value);
        break;
    case M64P_GL_GREEN_SIZE:
        format.setGreenBufferSize(Value);
        break;
    case M64P_GL_BLUE_SIZE:
        format.setBlueBufferSize(Value);
        break;
    case M64P_GL_ALPHA_SIZE:
        format.setAlphaBufferSize(Value);
        break;
    case M64P_GL_SWAP_CONTROL:
        format.setSwapInterval(Value);
        break;
    case M64P_GL_MULTISAMPLEBUFFERS:
        break;
    case M64P_GL_MULTISAMPLESAMPLES:
        format.setSamples(Value);
        break;
    case M64P_GL_CONTEXT_MAJOR_VERSION:
        format.setMajorVersion(Value);
        break;
    case M64P_GL_CONTEXT_MINOR_VERSION:
        format.setMinorVersion(Value);
        break;
    case M64P_GL_CONTEXT_PROFILE_MASK:
        switch (Value)
        {
        case M64P_GL_CONTEXT_PROFILE_CORE:
            format.setProfile(QSurfaceFormat::CoreProfile);
            break;
        case M64P_GL_CONTEXT_PROFILE_COMPATIBILITY:
            format.setProfile(QSurfaceFormat::CompatibilityProfile);
            break;
        case M64P_GL_CONTEXT_PROFILE_ES:
            format.setRenderableType(QSurfaceFormat::OpenGLES);
            break;
        }

        break;
    }

    return M64ERR_SUCCESS;
}

m64p_error VidExt_GLGetAttr(m64p_GLattr Attr, int *pValue)
{
    QSurfaceFormat::SwapBehavior SB = format.swapBehavior();
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
            format.alphaBufferSize() + format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize();
        break;
    case M64P_GL_DEPTH_SIZE:
        *pValue = format.depthBufferSize();
        break;
    case M64P_GL_RED_SIZE:
        *pValue = format.redBufferSize();
        break;
    case M64P_GL_GREEN_SIZE:
        *pValue = format.greenBufferSize();
        break;
    case M64P_GL_BLUE_SIZE:
        *pValue = format.blueBufferSize();
        break;
    case M64P_GL_ALPHA_SIZE:
        *pValue = format.alphaBufferSize();
        break;
    case M64P_GL_SWAP_CONTROL:
        *pValue = format.swapInterval();
        break;
    case M64P_GL_MULTISAMPLEBUFFERS:
        break;
    case M64P_GL_MULTISAMPLESAMPLES:
        *pValue = format.samples();
        break;
    case M64P_GL_CONTEXT_MAJOR_VERSION:
        *pValue = format.majorVersion();
        break;
    case M64P_GL_CONTEXT_MINOR_VERSION:
        *pValue = format.minorVersion();
        break;
    case M64P_GL_CONTEXT_PROFILE_MASK:
        switch (format.profile())
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

m64p_error VidExt_GLSwapBuf(void)
{
    if (renderThread != QThread::currentThread())
    {
        return M64ERR_UNSUPPORTED;
    }

    g_OGLWidget->context()->swapBuffers(g_OGLWidget);
    g_OGLWidget->context()->makeCurrent(g_OGLWidget);

    return M64ERR_SUCCESS;
}

m64p_error VidExt_SetCaption(const char *Title)
{
    g_EmuThread->on_VidExt_SetCaption(QString(Title));
    return M64ERR_SUCCESS;
}

m64p_error VidExt_ToggleFS(void)
{
    g_EmuThread->on_VidExt_ToggleFS();
    return M64ERR_SUCCESS;
}

m64p_error VidExt_ResizeWindow(int Width, int Height)
{
    g_EmuThread->on_VidExt_ResizeWindow(Width, Height);
    return M64ERR_SUCCESS;
}

uint32_t VidExt_GLGetDefaultFramebuffer(void)
{
    return g_OGLWidget->context()->defaultFramebufferObject();
}
