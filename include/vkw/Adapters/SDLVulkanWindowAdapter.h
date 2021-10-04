#ifndef VKW_SDL_VULKAN_WINDOW_ADAPTER_H
#define VKW_SDL_VULKAN_WINDOW_ADAPTER_H

#if __has_include(<SDL2/SDL2.h>)
#include <SDL2/SDL2.h>
#include <SDL2/SDL_vulkan.h>
#else
#include <SDL.h>
#include <SDL_vulkan.h>
#endif

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#include "VulkanWindowAdapter.h"

namespace vkw
{

struct SDLVulkanWindowAdapter : public VulkanWindowAdapater
{
    SDL_Window * m_window = nullptr;

    void createWindow(const char * title, int x, int y, int w, int h, Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN )
    {
        m_window = SDL_CreateWindow(title, x, y, w, h, flags);
    }

    void destroy()
    {
        if(m_window)
        {
            SDL_DestroyWindow(m_window);
        }
        m_window = nullptr;
    }
    //=================================================================================
    // These functions must be overidden window manager
    //=================================================================================
    VkSurfaceKHR createSurface(VkInstance instance) override
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        SDL_Vulkan_CreateSurface(m_window, instance, &surface);
        return surface;
    }
    /**
     * @brief getRequiredVulkanExtensions
     * @return
     *
     * Get the required vulkan extensions required to
     * be able to present to SDL windows
     */
    std::vector<std::string> getRequiredVulkanExtensions() override
    {
        std::vector<std::string> outExtensions;
        // Figure out the amount of extensions vulkan needs to interface with the os windowing system
        // This is necessary because vulkan is a platform agnostic API and needs to know how to interface with the windowing system
        unsigned int ext_count = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(m_window, &ext_count, nullptr))
        {
            std::runtime_error("Unable to query the number of Vulkan instance extensions");
            return {};
        }

        // Use the amount of extensions queried before to retrieve the names of the extensions
        std::vector<const char*> ext_names(ext_count);
        if (!SDL_Vulkan_GetInstanceExtensions(m_window, &ext_count, ext_names.data()))
        {
            std::runtime_error("Unable to query the number of Vulkan instance extension names");
        }

        // Display names
        //std::runtime_error("found " << ext_count << " Vulkan instance extensions:\n";
        for (unsigned int i = 0; i < ext_count; i++)
        {
            //std::cout << i << ": " << ext_names[i] << "\n";
            outExtensions.emplace_back(ext_names[i]);
        }

        // Add debug display extension, we need this to relay debug messages
        outExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        return outExtensions;
    }

    VkExtent2D getDrawableSize() override
    {
        int32_t  iwidth,iheight = 0;
        SDL_Vulkan_GetDrawableSize(m_window, &iwidth, &iheight);
        VkExtent2D ext;

        ext.width  = static_cast<uint32_t>(iwidth);
        ext.height = static_cast<uint32_t>(iheight);

        return ext;
    }

};


}

#endif
