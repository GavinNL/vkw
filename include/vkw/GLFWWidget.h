#ifndef VKW_GLFW_VULKAN_WIDGET3_H
#define VKW_GLFW_VULKAN_WIDGET3_H

#include "VKWVulkanWindow.h"
#include "Adapters/GLFWVulkanWindowAdapter.h"
#include "VulkanApplication.h"
#include "Frame.h"
#include <iostream>
#include <thread>

namespace vkw {


/**
 * @brief The SDLVulkanWidget struct
 *
 * The SDLVulkanWidget use an SDL_Window to provide
 * a drawing surface. This mimics the QtVulkanWidget
 * so that they can be used interchangeably without
 * much modification.
 */
class GLFWVulkanWidget : public VKWVulkanWindow
{
public:
    struct CreateInfo
    {
        std::string                windowTitle;
        uint32_t                   width;
        uint32_t                   height;
        InstanceInitilizationInfo2 instanceInfo;
        DeviceInitilizationInfo2   deviceInfo;
        SurfaceInitilizationInfo2  surfaceInfo;
    };

    ~GLFWVulkanWidget()
    {
    }
    GLFWVulkanWindowAdapter * m_adapter = nullptr;
    CreateInfo m_createInfo;

    void create(CreateInfo const &C)
    {
        m_createInfo = C;

        m_adapter = new GLFWVulkanWindowAdapter();

        m_adapter->createWindow( m_createInfo.windowTitle.c_str(),
                     static_cast<int>(m_createInfo.width),
                     static_cast<int>(m_createInfo.height));

        setWindowAdapater(m_adapter);

        createVulkanInstance(m_createInfo.instanceInfo);

        createVulkanSurface(m_createInfo.surfaceInfo);

        createVulkanDevice(m_createInfo.deviceInfo);
    }

    void finalize(Application * app)
    {
        app->releaseSwapChainResources();
        app->releaseResources();
    }

    void render( Application * app)
    {
        auto fr = acquireNextFrame();

        fr.beginCommandBuffer();

        app->m_renderNextFrame = false;
        app->render(fr);

        fr.endCommandBuffer();
        frameReady(fr);
    }

    void _initSwapchainVars(Application * app)
    {
        app->m_swapChainSize       = getSwapchainExtent();
        app->m_swapChainFormat     = getSwapchainFormat();
        app->m_swapChainDepthFormat= getDepthFormat();
        app->m_concurrentFrameCount= static_cast<uint32_t>(m_swapchainFrameBuffers.size());
        app->m_defaultRenderPass   = m_renderPass;

        app->m_swapchainImageViews = m_swapchainImageViews;
        app->m_swapchainImages = m_swapchainImages;

        app->m_currentSwapchainIndex=0;
    }

    int exec(Application * app)
    {
        return exec(app, [](){});
    }
    /**
     * @brief exec
     * @return
     *
     * Similar to Qt's app.exec(). this will
     * loop until the the windows is closed
     */
    template<typename SDL_MAIN_LOOP_CALLABLE>
    int exec(Application * app, SDL_MAIN_LOOP_CALLABLE && mainLoop)
    {
        app->m_device         = getDevice();
        app->m_physicalDevice = getPhysicalDevice();
        app->m_instance       = getInstance();

        app->m_graphicsQueue  = getGraphicsQueue();
        app->m_presentQueue   = getPresentQueue();
        app->m_graphicsQueueIndex = getGraphicsQueueIndex();
        app->m_presentQueueIndex  = getPresentQueueIndex();

        _initSwapchainVars(app);

        app->initResources();
        app->initSwapChainResources();

        while( !glfwWindowShouldClose(m_adapter->m_window) )
        {
            glfwPollEvents();
            bool resize = m_adapter->requiresResize();

            if( app->shouldQuit() )
            {
                break;
            }
            if(resize)
            {
                app->releaseSwapChainResources();
                rebuildSwapchain();

                _initSwapchainVars(app);
                app->initSwapChainResources();

                m_adapter->clearRequireResize();
            }


            if( app->shouldRender() )
            {
                mainLoop();
                render(app);
            }
        }

        app->releaseSwapChainResources();
        app->releaseResources();
        destroy();

        return 0;
    }

    /**
     * @brief frameReady
     *
     * Call this function to present the frame.
     */
    void frameReady(Frame & fr)
    {
        submitFrame(fr);
        presentFrame(fr);
        waitForPresent();
    }
};

}

#endif
