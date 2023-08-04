#include "screen.h"
#include "gfx_m64p.h"
#include <stddef.h>
#include <stdbool.h>

/* global variables */
bool window_fullscreen;
bool window_widescreen;
uint32_t window_width;
uint32_t window_height;
bool window_vsync;

/* definitions of pointers to Core video extension functions */
static ptr_VidExt_InitWithRenderMode CoreVideo_InitWithRenderMode = NULL;
static ptr_VidExt_Quit CoreVideo_Quit = NULL;
static ptr_VidExt_SetCaption CoreVideo_SetCaption = NULL;
static ptr_VidExt_ToggleFullScreen CoreVideo_ToggleFullScreen = NULL;
static ptr_VidExt_ResizeWindow CoreVideo_ResizeWindow = NULL;
static ptr_VidExt_VK_GetSurface CoreVideo_VK_GetSurface = NULL;
static ptr_VidExt_VK_GetInstanceExtensions CoreVideo_VK_GetInstanceExtensions = NULL;
static ptr_VidExt_SetVideoMode CoreVideo_SetVideoMode = NULL;
static ptr_VidExt_GL_SwapBuffers CoreVideo_SwapCounter = NULL;

bool screen_init()
{
    /* Get the core Video Extension function pointers from the library handle */
    CoreVideo_InitWithRenderMode = (ptr_VidExt_InitWithRenderMode)DLSYM(CoreLibHandle, "VidExt_InitWithRenderMode");
    CoreVideo_Quit = (ptr_VidExt_Quit)DLSYM(CoreLibHandle, "VidExt_Quit");
    CoreVideo_SetCaption = (ptr_VidExt_SetCaption)DLSYM(CoreLibHandle, "VidExt_SetCaption");
    CoreVideo_ToggleFullScreen = (ptr_VidExt_ToggleFullScreen)DLSYM(CoreLibHandle, "VidExt_ToggleFullScreen");
    CoreVideo_ResizeWindow = (ptr_VidExt_ResizeWindow)DLSYM(CoreLibHandle, "VidExt_ResizeWindow");
    CoreVideo_VK_GetSurface = (ptr_VidExt_VK_GetSurface)DLSYM(CoreLibHandle, "VidExt_VK_GetSurface");
    CoreVideo_VK_GetInstanceExtensions = (ptr_VidExt_VK_GetInstanceExtensions)DLSYM(CoreLibHandle, "VidExt_VK_GetInstanceExtensions");
    CoreVideo_SetVideoMode = (ptr_VidExt_SetVideoMode)DLSYM(CoreLibHandle, "VidExt_SetVideoMode");
    CoreVideo_SwapCounter = (ptr_VidExt_GL_SwapBuffers)DLSYM(CoreLibHandle, "VidExt_GL_SwapBuffers");

    if (CoreVideo_InitWithRenderMode(M64P_RENDER_VULKAN) != M64ERR_SUCCESS)
    {
        return false;
    }

    CoreVideo_SetCaption("Mupen64Plus-Parallel");
    return true;
}

bool screen_set_mode(bool fullscreen)
{
    return CoreVideo_SetVideoMode(window_width, window_height, 0, fullscreen ? M64VIDEO_FULLSCREEN : M64VIDEO_WINDOWED, M64VIDEOFLAG_SUPPORT_RESIZING) == M64ERR_SUCCESS;
}

void screen_close()
{
    CoreVideo_Quit();
}

void screen_toggle_fullscreen()
{
    CoreVideo_ToggleFullScreen();
}

m64p_error screen_get_surface(VkInstance instance, VkSurfaceKHR* vkSurface)
{
    return CoreVideo_VK_GetSurface((void**)vkSurface, (void*)instance);
}

m64p_error screen_get_instance_extensions(const char ** ext[], uint32_t* ext_num)
{
    return CoreVideo_VK_GetInstanceExtensions(ext, ext_num);
}
