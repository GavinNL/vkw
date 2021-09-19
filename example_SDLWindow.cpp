#include <iostream>
#include <vkw/SDLVulkanWindow.h>
#include <vkw/SDLVulkanWindowAdapter.h>

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

#if defined(__WIN32__)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    // This needs to be called first to initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // create a default window and initialize all vulkan
    // objects.
    auto window = new vkw::VKWVulkanWindow();
    auto sdl_window = new vkw::SDLVulkanWindowAdapter();

    // 1. create the window and set the adapater
    sdl_window->createWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,768);
    window->setWindowAdapater(sdl_window);

    // 2. Create the Instance
    vkw::VKWVulkanWindow::InstanceInitilizationInfo2 instanceInfo;
    instanceInfo.debugCallback = &VulkanReportFunc;
    instanceInfo.vulkanVersion = VK_MAKE_VERSION(1, 0, 0);
    instanceInfo.enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    window->createVulkanInstance(instanceInfo);

    // 3. Create the surface
    vkw::VKWVulkanWindow::SurfaceInitilizationInfo2 surfaceInfo;
    surfaceInfo.depthFormat          = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
    surfaceInfo.presentMode          = VK_PRESENT_MODE_FIFO_KHR;
    surfaceInfo.additionalImageCount = 1;// how many additional swapchain images should we create ( total = min_images + additionalImageCount
    window->createVulkanSurface(surfaceInfo);

    window->createVulkanPhysicalDevice();

    // 4. Create the device
    //    and add additional extensions that we want to enable
    vkw::VKWVulkanWindow::DeviceInitilizationInfo2 deviceInfo;
    deviceInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    deviceInfo.deviceExtensions.push_back(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

    // enable a new extended feature
    VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexState = {};
    dynamicVertexState.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
    dynamicVertexState.vertexInputDynamicState  = true;
    deviceInfo.enabledFeatures12.pNext          = &dynamicVertexState;

    window->createVulkanDevice(deviceInfo);



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



    // delete the window to destroy all objects
    // that were created.
    window->destroy();
    delete window;

    SDL_Quit();
    return 0;
}

#include <vkw/SDLVulkanWindow_INIT.inl>
#include <vkw/SDLVulkanWindow_USAGE.inl>
