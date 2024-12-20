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
#include <RMG-Core/Core.hpp>

#include "OnScreenDisplay.hpp"

#include <QApplication>
#include <QOpenGLContext>
#include <QVulkanInstance>
#include <QThread>
#include <QScreen>

//
// Local Variables
//

static Thread::EmulationThread* l_EmuThread             = nullptr;
static UserInterface::MainWindow* l_MainWindow          = nullptr;
static UserInterface::Widget::OGLWidget** l_OGLWidget   = nullptr;
static UserInterface::Widget::VKWidget** l_VulkanWidget = nullptr;
static QThread* l_RenderThread                          = nullptr;
static bool l_OpenGLInitialized                         = false;
static bool l_OsdInitialized                            = false;
static QSurfaceFormat l_SurfaceFormat;
static m64p_render_mode l_RenderMode;

static QVulkanInstance l_VulkanInstance;
static QVulkanInfoVector<QVulkanExtension> l_VulkanExtensions;
static QVector<const char*> l_VulkanExtensionList;

//
// VidExt Functions
//

static bool VidExt_OglSetup(void)
{
    l_EmuThread->on_VidExt_SetupOGL(l_SurfaceFormat, QThread::currentThread());

    while (!(*l_OGLWidget)->isVisible())
    {
        continue;
    }

    if (!(*l_OGLWidget)->GetContext()->isValid())
    {
        CoreAddCallbackMessage(CoreDebugMessageType::Error, "Failed to retrieve valid OpenGL context");
        return false;
    }

    if (!(*l_OGLWidget)->GetContext()->makeCurrent((*l_OGLWidget)))
    {
        CoreAddCallbackMessage(CoreDebugMessageType::Error, "Failed to make OpenGL context current");
        return false;
    }

    l_OpenGLInitialized = true;
    return true;
}

static m64p_error VidExt_InitWithRenderMode(m64p_render_mode RenderMode)
{
    l_RenderMode = RenderMode;
    l_RenderThread = QThread::currentThread();

    if (RenderMode == M64P_RENDER_OPENGL)
    {
        l_SurfaceFormat = QSurfaceFormat::defaultFormat();
        l_SurfaceFormat.setOption(QSurfaceFormat::DeprecatedFunctions, 1);
        l_SurfaceFormat.setDepthBufferSize(24);
        l_SurfaceFormat.setProfile(QSurfaceFormat::CompatibilityProfile);
        l_SurfaceFormat.setSwapInterval(0);
        if (l_SurfaceFormat.renderableType() != QSurfaceFormat::OpenGLES)
        {
            l_SurfaceFormat.setMajorVersion(3);
            l_SurfaceFormat.setMinorVersion(3);
        }
        else
        {
            l_SurfaceFormat.setMajorVersion(2);
            l_SurfaceFormat.setMinorVersion(0);
        }
    }

    l_EmuThread->on_VidExt_Init(RenderMode == M64P_RENDER_OPENGL ? VidExtRenderMode::OpenGL : VidExtRenderMode::Vulkan);

    return M64ERR_SUCCESS;
}

static m64p_error VidExt_Init(void)
{
    return VidExt_InitWithRenderMode(M64P_RENDER_OPENGL);
}

static m64p_error VidExt_Quit(void)
{
    OnScreenDisplayShutdown();

    if (l_RenderMode == M64P_RENDER_OPENGL)
    {
        // move OpenGL context back to the GUI thread
        (*l_OGLWidget)->MoveContextToThread(QApplication::instance()->thread());
    }
    else
    {
        // remove vulkan instance from widget
        // and destroy the instance
        (*l_VulkanWidget)->setVulkanInstance(nullptr);
        if (l_VulkanInstance.isValid())
        {
            l_VulkanInstance.destroy();
        }
    }
    l_EmuThread->on_VidExt_Quit();

    l_OpenGLInitialized = false;
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
    // initialize OpenGL when render mode
    // is OpenGL and not Vulkan
    if (l_RenderMode == M64P_RENDER_OPENGL)
    {
        if (!l_OpenGLInitialized && !VidExt_OglSetup())
        {
            return M64ERR_SYSTEM_FAIL;
        }

        // try to initialize the OSD
        // when opengl 3 is used
        if (l_SurfaceFormat.majorVersion() == 3 &&
            !l_OsdInitialized)
        {
            if (!OnScreenDisplayInit())
            {
                CoreAddCallbackMessage(CoreDebugMessageType::Error, "Failed to initialize OSD");
                return M64ERR_SYSTEM_FAIL;
            }

            OnScreenDisplayLoadSettings();
            l_OsdInitialized = true;
        }
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
    return M64ERR_UNSUPPORTED;
}

static m64p_function VidExt_GLGetProc(const char *Proc)
{
    if (l_RenderMode != M64P_RENDER_OPENGL)
    {
        return nullptr;
    }

    return (*l_OGLWidget)->GetContext()->getProcAddress(Proc);
}

static m64p_error VidExt_GLSetAttr(m64p_GLattr Attr, int Value)
{
    if (l_RenderMode != M64P_RENDER_OPENGL)
    {
        return M64ERR_INVALID_STATE;
    }

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
    if (l_RenderMode != M64P_RENDER_OPENGL)
    {
        return M64ERR_INVALID_STATE;
    }

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
    if (l_RenderMode != M64P_RENDER_OPENGL)
    {
        return M64ERR_INVALID_STATE;
    }

    if (l_RenderThread != QThread::currentThread())
    {
        return M64ERR_UNSUPPORTED;
    }

    OnScreenDisplayRender();

    (*l_OGLWidget)->GetContext()->swapBuffers((*l_OGLWidget));
    (*l_OGLWidget)->GetContext()->makeCurrent((*l_OGLWidget));

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
    if (l_RenderMode != M64P_RENDER_OPENGL)
    {
        return 0;
    }

    return (*l_OGLWidget)->GetContext()->defaultFramebufferObject();
}

static m64p_error VidExt_VK_GetSurface(void** Surface, void* Instance)
{
    if (l_RenderMode != M64P_RENDER_VULKAN)
    {
        return M64ERR_INVALID_STATE;
    }

    // we don't support receiving a null handle
    // for the VkInstance
    if ((VkInstance)Instance == VK_NULL_HANDLE)
    {
        return M64ERR_UNSUPPORTED;
    }

    // use VkInstance from plugin
    // when we don't have a VkInstance yet
    if (l_VulkanInstance.vkInstance() == VK_NULL_HANDLE)
    {
        l_VulkanInstance.setVkInstance((VkInstance)Instance);
        if (!l_VulkanInstance.create())
        {
            CoreAddCallbackMessage(CoreDebugMessageType::Error, "Failed to create vulkan instance");
            return M64ERR_SYSTEM_FAIL;
        }
        (*l_VulkanWidget)->setVulkanInstance(&l_VulkanInstance);
    }

    // attempt to retrieve vulkan surface for window
    VkSurfaceKHR vulkanSurface = QVulkanInstance::surfaceForWindow((*l_VulkanWidget));
    if (vulkanSurface == VK_NULL_HANDLE)
    {
        CoreAddCallbackMessage(CoreDebugMessageType::Error, "Failed to retrieve vulkan surface for window");
        return M64ERR_SYSTEM_FAIL;
    }

    *Surface = (void*)vulkanSurface;
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_VK_GetInstanceExtensions(const char** Extensions[], uint32_t* NumExtensions)
{
    if (l_RenderMode != M64P_RENDER_VULKAN)
    {
        return M64ERR_INVALID_STATE;
    }

    l_VulkanExtensions = l_VulkanInstance.supportedExtensions();
    l_VulkanExtensionList.clear();

    // add every extension to the string list
    for (int i = 0; i < l_VulkanExtensions.size(); i++)
    {
        l_VulkanExtensionList.append(l_VulkanExtensions[i].name.data());
    }

    *Extensions    = l_VulkanExtensionList.data();
    *NumExtensions = l_VulkanExtensions.size();
    return M64ERR_SUCCESS;
}

//
// Exported Functions
//

bool SetupVidExt(Thread::EmulationThread* emuThread, UserInterface::MainWindow* mainWindow, 
    UserInterface::Widget::OGLWidget** oglWidget, UserInterface::Widget::VKWidget** vulkanWidget)
{
    l_EmuThread    = emuThread;
    l_MainWindow   = mainWindow;
    l_OGLWidget    = oglWidget;
    l_VulkanWidget = vulkanWidget;

    m64p_video_extension_functions vidext_funcs;

    vidext_funcs.Functions = 17;
    vidext_funcs.VidExtFuncInit = &VidExt_Init;
    vidext_funcs.VidExtFuncInitWithRenderMode = &VidExt_InitWithRenderMode;
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
    vidext_funcs.VidExtFuncVKGetSurface = &VidExt_VK_GetSurface;
    vidext_funcs.VidExtFuncVKGetInstanceExtensions = &VidExt_VK_GetInstanceExtensions;

    return CoreSetupVidExt(vidext_funcs);
}


