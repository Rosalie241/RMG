#include "wsi_platform.hpp"
#include "screen.h"
#include "gfx_m64p.h"

VkSurfaceKHR MupenWSIPlatform::create_surface(VkInstance instance, VkPhysicalDevice gpu)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (screen_get_surface(instance, &surface) != M64ERR_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    return surface;
}

void MupenWSIPlatform::destroy_surface(VkInstance instance, VkSurfaceKHR surface)
{
}

std::vector<const char *> MupenWSIPlatform::get_instance_extensions()
{
    std::vector<const char*> extensionsList;
    const char** extensionNames = nullptr;
    uint32_t extensionCount = 0;

    if (screen_get_instance_extensions(&extensionNames, &extensionCount) != M64ERR_SUCCESS)
    {
        return extensionsList;
    }

    for (int i = 0; i < extensionCount; i++)
    {
        extensionsList.push_back(extensionNames[i]);
    }

    return extensionsList;
}

uint32_t MupenWSIPlatform::get_surface_width()
{
    return window_width;
}

uint32_t MupenWSIPlatform::get_surface_height()
{
    return window_height;
}

bool MupenWSIPlatform::alive(Vulkan::WSI &wsi)
{
    return true;
}

void MupenWSIPlatform::poll_input()
{
}

void MupenWSIPlatform::poll_input_async(Granite::InputTrackerHandler *handler)
{
}

void MupenWSIPlatform::do_resize()
{
    resize = true;
}