#include <iostream>

//#define VKW_WINDOW_LIB 1

#if VKW_WINDOW_LIB == 1
#include <vkw/SDLWidget.h>
#elif VKW_WINDOW_LIB == 2
#include <vkw/GLFWWidget.h>
#endif

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
// rendering code for vulkan. It can be used:
// GLFWVulkanWidget,
// SDLVulkanWidget,
// QTVulkanWidget
//
#include "example_myApplication.h"

int MAIN(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    // create a vulkan window widget
#if VKW_WINDOW_LIB == 1
    using WidgetType = vkw::SDLVulkanWidget;
#elif VKW_WINDOW_LIB == 2
    using WidgetType = vkw::GLFWVulkanWidget;
#endif

    WidgetType vulkanWindow;

    // set the initial properties of the
    // window. Also specify that we want
    // a depth stencil attachment
    WidgetType::CreateInfo c;
    c.width       = 1024;
    c.height      = 768;
    c.windowTitle = "My Vulkan Application Window";

    c.surfaceInfo.depthFormat    = VK_FORMAT_D32_SFLOAT_S8_UINT;
    c.instanceInfo.debugCallback = &VulkanReportFunc;
    c.instanceInfo.vulkanVersion = VK_MAKE_VERSION(1,2,0);

    c.deviceInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    c.deviceInfo.deviceExtensions.push_back(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

    // enable a new extended feature
    //VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexState = {};
    //dynamicVertexState.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
    //dynamicVertexState.vertexInputDynamicState  = true;
    //c.deviceInfo.enabledFeatures12.pNext         = &dynamicVertexState;


    // Here is the actual vulkan application that does
    // all the rendering.
    MyApplication app;


    {
        #if VKW_WINDOW_LIB == 1
            // This needs to be called first to initialize SDL
            SDL_Init(SDL_INIT_EVERYTHING);

            vulkanWindow.create(c);

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

        #elif VKW_WINDOW_LIB == 2

            glfwInit();

            vulkanWindow.create(c);

            // put the window in the main loop
            // GLFW requires you to register callbacks
            // for input events. you will have to do these yourself
            vulkanWindow.exec(&app);

            vulkanWindow.destroy();

            glfwTerminate();
        #endif
    }

    return 0;
}


#if VKW_WINDOW_LIB == 1

    #if defined(__WIN32__)
    int SDL_main(int argc, char *argv[])
    #else
    int main(int argc, char *argv[])
    #endif
    {
        return MAIN(argc, argv);
    }
#elif VKW_WINDOW_LIB == 2
    int main(int argc, char *argv[])
    {
        return MAIN(argc, argv);
    }
#endif


#include <vkw/VKWVulkanWindow.inl>
