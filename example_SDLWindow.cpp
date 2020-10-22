#include <iostream>
#include <vkw/SDLVulkanWindow.h>

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

int main()
{
    // This needs to be called first to initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // create a default window and initialize all vulkan
    // objects.
    auto window = new vkw::SDLVulkanWindow();

    // 1. create the window
    window->createWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,768);

    // 2. initialize the vulkan instance
    vkw::SDLVulkanWindow::InitilizationInfo info;

    info.callback = VulkanReportFunc;
    // info.enabledLayers.clear();     // clear the default layers/extensions if you do not want them.
    // info.enabledExtensions.clear(); // clear the default layers/extensions if you do not want them.

    window->createVulkanInstance( info);


    {
        auto ext = window->getAvailableVulkanExtensions();
        for(auto & e : ext)
        {
            std::cout << "Extension: " << e << std::endl;
        }
    }
    {
        auto ext = window->getAvailableVulkanLayers();
        for(auto & e : ext)
        {
            std::cout << "Layers: " << e << std::endl;
        }
    }

    // 3. Create the following objects:
    //    instance, physical device, device, graphics/present queues,
    //    swap chain, depth buffer, render pass and framebuffers
    vkw::SDLVulkanWindow::SurfaceInitilizationInfo surfaceInfo;

    //surfaceInfo.depthFormat = VkFormat::VK_FORMAT_UNDEFINED; // set to undefined to disable depth image creation
    surfaceInfo.additionalImageCount = 1; // create one additional image in the swap chain for triple buffering.
    surfaceInfo.enabledFeatures.tessellationShader = 1;

    // Only call this once.
    window->initSurface(surfaceInfo);


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
    delete window;

    SDL_Quit();
    return 0;
}

#include <vkw/SDLVulkanWindow_INIT.inl>
#include <vkw/SDLVulkanWindow_USAGE.inl>
