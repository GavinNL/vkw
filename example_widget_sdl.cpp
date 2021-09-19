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


// This header file contains the actual
// rendering code for vulkan. It can be used
// by both the SDLWidget and the Qt widget.
// see example_widget_sdl.cpp
// and example_widget_qt.cpp
#include "example_myApplication.h"


#if defined(__WIN32__)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    (void)argc;
    (void)argv;
    // This needs to be called first to initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // create a vulkan window widget
    vkw::SDLVulkanWidget vulkanWindow;

    // set the initial properties of the
    // window. Also specify that we want
    // a depth stencil attachment
    vkw::SDLVulkanWidget::CreateInfo c;
    c.width       = 1024;
    c.height      = 768;
    c.windowTitle = "My Vulkan Application Window";

    c.surfaceInfo.depthFormat    = VK_FORMAT_D32_SFLOAT_S8_UINT;
    c.instanceInfo.debugCallback = &VulkanReportFunc;

    c.deviceInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    c.deviceInfo.deviceExtensions.push_back(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

    // enable a new extended feature
    //VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexState = {};
    //dynamicVertexState.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
    //dynamicVertexState.vertexInputDynamicState  = true;
    //c.deviceInfo.enabledFeatures12.pNext         = &dynamicVertexState;

    // create the window and initialize
    vulkanWindow.create(c);


    MyApplication app;

    // put the window in the main loop
    // and provide a callback function for the SDL events
    vulkanWindow.exec(&app,
                      [&app](SDL_Event const & evt)
    {
        if( evt.type == SDL_QUIT)
            app.quit();
    });

    vulkanWindow.destroy();
    SDL_Quit();
    return 0;
}

#include <vkw/SDLVulkanWindow_INIT.inl>
#include <vkw/SDLVulkanWindow_USAGE.inl>
