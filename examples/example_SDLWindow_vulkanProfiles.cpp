#include <iostream>
#include <vkw/VKWVulkanWindow.h>
#include <vkw/Adapters/SDLVulkanWindowAdapter.h>
#include <cassert>
#include <vkw/VKWVulkanWindowProfile.h>
#include <vulkan/vk_format_utils.h>

// callback function for validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanReportFunc(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    printf("VULKAN VALIDATION: [%s] %s\n", layerPrefix, msg);

    return VK_FALSE;
}

//#define PROFILE_NAME         VP_LUNARG_DESKTOP_PORTABILITY_2021_NAME
//#define PROFILE_SPEC_VERSION VP_LUNARG_DESKTOP_PORTABILITY_2021_SPEC_VERSION

#define PROFILE_NAME         VP_KHR_ROADMAP_2022_NAME
#define PROFILE_SPEC_VERSION VP_KHR_ROADMAP_2022_SPEC_VERSION




#if defined(__WIN32__)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    auto window = new vkw::VKWVulkanWindowProfile();
    auto sdl_window = new vkw::SDLVulkanWindowAdapter();
    sdl_window->createWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,768);
    window->setWindowAdapater(sdl_window);
    //-------------------------------------------------------------



    //-------------------------------------------------------------
    // Vulkan Initialization part
    //-------------------------------------------------------------
    const VpProfileProperties profile_properties = {PROFILE_NAME, PROFILE_SPEC_VERSION};

    // first get the required extensions we need for the window
    auto requiredInstanceExtensions = sdl_window->getRequiredVulkanExtensions();
    requiredInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    // create a vulkan instance using the profiles
    window->createProfileInstance(profile_properties, requiredInstanceExtensions, {"VK_LAYER_KHRONOS_validation"});

    // set the debug callback function
    window->setDebugCallback(&VulkanReportFunc);

    // create the surface we want to use.
    // This is dependent on the window manager
    auto surface = sdl_window->createSurface(window->getInstance());

    // get a physical device suitable for presenting
    // to the surface
    auto physicalDevice = window->getPhysicalDevice(window->getInstance(), surface);

    window->createProfileDevice(physicalDevice, surface, profile_properties, {VK_KHR_SWAPCHAIN_EXTENSION_NAME});
    //-------------------------------------------------------------

    bool running=true;
    while(running)
    {
        SDL_Event event;
        bool resize=false;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
                    /*&& event.window.windowID == SDL_GetWindowID( window->getSDLWindow()) */ )
            {
                resize=true;
            }
        }
        if( resize )
        {
            // If the window has changed size. we need to rebuild the swapchain
            // and any other textures (depth texture)
            window->rebuildSwapchain();
        }

        // Get the next available frame.
        // the Frame struct is simply a POD containing
        // all the information that you need to record a command buffer
        auto frame = window->acquireNextFrame();


        frame.beginCommandBuffer();
            frame.clearColor = {{1.f,0.f,0.f,0.f}};
            frame.beginRenderPass( frame.commandBuffer );


            // record to frame.commandbuffer

            frame.endRenderPass(frame.commandBuffer);
        frame.endCommandBuffer();

        window->submitFrame(frame);

        // Present the frame after you have recorded
        // the command buffer;
        window->presentFrame(frame);
        window->waitForPresent();
    }


    window->destroy();
    sdl_window->destroy();
    delete window;
    delete sdl_window;

}


#include <vkw/VKWVulkanWindow.inl>
