#include <iostream>
#include <vkw/SDLWidget.h>

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
    std::cout << "VULKAN VALIDATION: " << layerPrefix << " :: " <<  msg << std::endl;;

    return VK_FALSE;
}


class MyApplication : public Application
{
    // Application interface
public:
    void initResources()
    {

    }
    void releaseResources()
    {
    }
    void initSwapChainResources()
    {

    }
    void releaseSwapChainResources()
    {

    }
    void render(Frame &frame)
    {
        assert( frame.depthImage != VK_NULL_HANDLE);
        frame.beginRenderPass( frame.commandBuffer );

        frame.endRenderPass(frame.commandBuffer);

        // request the next frame
        // so that this function will be called again
        requestNextFrame();
    }
};

int main()
{
    // This needs to be called first to initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // create a vulkan window widget
    SDLVulkanWidget3 vulkanWindow;

    SDLVulkanWidget3::CreateInfo c;
    c.width       = 1024;
    c.height      = 768;
    c.windowTitle = "My Vulkan Application Window";
    c.depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
    c.callback    = &VulkanReportFunc;


    // create the window and initialize
    vulkanWindow.create(c);


    MyApplication app;
    vulkanWindow.exec(&app);

    vulkanWindow.destroy();
    SDL_Quit();
    return 0;
}


#include <vkw/SDLVulkanWindow_INIT.inl>
#include <vkw/SDLVulkanWindow_USAGE.inl>
