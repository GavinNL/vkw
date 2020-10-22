#ifndef QT_SDL_WIDGET3_H
#define QT_SDL_WIDGET3_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.hpp>

#include "SDLVulkanWindow.h"
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
class SDLVulkanWidget : public SDLVulkanWindow
{
public:
    struct CreateInfo  : public InitilizationInfo,
                         public SurfaceInitilizationInfo
    {
        std::string windowTitle;
        uint32_t width;
        uint32_t height;
    };

    ~SDLVulkanWidget()
    {
    }

    CreateInfo m_createInfo;
    void create(CreateInfo &C)
    {
        m_createInfo = C;

        createWindow( C.windowTitle.c_str(),
                     SDL_WINDOWPOS_CENTERED,
                     SDL_WINDOWPOS_CENTERED,
                     static_cast<int>(C.width),
                     static_cast<int>(C.height));

        createVulkanInstance(C);

        initSurface(C);
    }

    void finalize(Application * app)
    {
        app->releaseSwapChainResources();
        app->releaseResources();
    }

    template<typename callable_t>
    void  poll( Application * app, callable_t && c)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            c(event);
            app->nativeWindowEvent(&event);
        }
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
        app->m_swapchainImages.clear();

        app->m_currentSwapchainIndex=0;
    }
    /**
     * @brief exec
     * @return
     *
     * Similar to Qt's app.exec(). this will
     * loop until the the windows is closed
     */
    template<typename SDL_EVENT_CALLABLE>
    int exec(Application * app, SDL_EVENT_CALLABLE && callable)
    {
        app->m_device         = getDevice();
        app->m_physicalDevice = getPhysicalDevice();
        app->m_instance       = getInstance();

        _initSwapchainVars(app);

        app->initResources();
        app->initSwapChainResources();

        while( true )
        {
            bool resize=false;
            poll(app, [&resize,&callable](SDL_Event const &E)
            {
                if (E.type == SDL_WINDOWEVENT && E.window.event == SDL_WINDOWEVENT_RESIZED
                        /*&& event.window.windowID == SDL_GetWindowID( window->getSDLWindow()) */ )
                {
                    resize=true;
                }
                callable(E);
            });

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
            }


            if( app->shouldRender() )
            {
                render(app);
            }

            std::this_thread::sleep_for( std::chrono::milliseconds(16));
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

