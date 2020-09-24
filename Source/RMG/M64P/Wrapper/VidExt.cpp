#include "VidExt.hpp"
#include "../../Globals.hpp"

#include <QThread>
#include <QOpenGLContext>
#include <iostream>
#include <QApplication>

static QSurfaceFormat format;

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
    std::cout << __FUNCTION__ << std::endl;

    format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::DeprecatedFunctions, 1);
    format.setDepthBufferSize(24);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setMajorVersion(2);
    format.setMinorVersion(1);

    g_EmuThread->on_VidExt_Init();

    return M64ERR_SUCCESS;
}

m64p_error VidExt_Quit(void)
{
    std::cout << __FUNCTION__ << std::endl;

    g_OGLWidget->SetThread(QApplication::instance()->thread());
    g_EmuThread->on_VidExt_Quit();
    ogl_setup = false;

    return M64ERR_SUCCESS;
}

m64p_error VidExt_ListModes(m64p_2d_size *SizeArray, int *NumSizes)
{
    std::cout << __FUNCTION__ << std::endl;

    SizeArray[0].uiHeight = 1080;
    SizeArray[0].uiWidth = 1920;
    *NumSizes = 1;

    return M64ERR_SUCCESS;
}

m64p_error VidExt_ListRates(m64p_2d_size Size, int *NumRates, int *Rates)
{
    std::cout << __FUNCTION__ << std::endl;

    Rates[0] = 60;
    *NumRates = 1;

    return M64ERR_SUCCESS;
}

m64p_error VidExt_SetMode(int Width, int Height, int BitsPerPixel, int ScreenMode, int Flags)
{
    if (!ogl_setup)
        VidExt_OglSetup();

    std::cout << __FUNCTION__ << std::endl;
    g_EmuThread->on_VidExt_SetMode(Width, Height, BitsPerPixel, ScreenMode, Flags);
    return M64ERR_SUCCESS;
}

m64p_error VidExt_SetModeWithRate(int Width, int Height, int RefreshRate, int BitsPerPixel, int ScreenMode, int Flags)
{
    if (!ogl_setup)
        VidExt_OglSetup();

    std::cout << __FUNCTION__ << std::endl;
    g_EmuThread->on_VidExt_SetModeWithRate(Width, Height, RefreshRate, BitsPerPixel, ScreenMode, Flags);
    return M64ERR_SUCCESS;
}

m64p_function VidExt_GLGetProc(const char *Proc)
{
    std::cout << __FUNCTION__ << std::endl;
    return g_OGLWidget->context()->getProcAddress(Proc);
}

m64p_error VidExt_GLSetAttr(m64p_GLattr Attr, int Value)
{
    std::cout << __FUNCTION__ << std::endl;

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
    std::cout << __FUNCTION__ << std::endl;
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
        *pValue = format.alphaBufferSize() + format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize();
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
    // std::cout << __FUNCTION__ << std::endl;

    g_OGLWidget->context()->swapBuffers(g_OGLWidget->context()->surface());
    //g_OGLWidget->context()->makeCurrent(g_OGLWidget->context()->surface());

    return M64ERR_SUCCESS;
}

m64p_error VidExt_SetCaption(const char *Title)
{
    std::cout << __FUNCTION__ << std::endl;
    g_EmuThread->on_VidExt_SetCaption(QString(Title));
    return M64ERR_SUCCESS;
}

m64p_error VidExt_ToggleFS(void)
{
    std::cout << __FUNCTION__ << std::endl;
    g_EmuThread->on_VidExt_ToggleFS();
    return M64ERR_SUCCESS;
}

m64p_error VidExt_ResizeWindow(int Width, int Height)
{
    std::cout << "VidExt_ResizeWindow" << std::endl;
    g_EmuThread->on_VidExt_ResizeWindow(Width, Height);
    return M64ERR_SUCCESS;
}

uint32_t VidExt_GLGetDefaultFramebuffer(void)
{
    std::cout << __FUNCTION__ << std::endl;
    return 0;
}
