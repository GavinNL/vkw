#ifndef VWK_MY_TEST_APPLICATION_H
#define VWK_MY_TEST_APPLICATION_H


#include <vkw/VulkanApplication.h>
#include <iostream>
#include <cassert>

class MyApplication : public vkw::Application
{
    // Application interface
public:
    void initResources() override
    {
        // The following can be used here
        // getDevice();
        // getPhysicalDevice();
        // getInstance();
        std::cout << "initResources() " << std::endl;
    }
    void releaseResources() override
    {
        // The following can be used here
        // getDevice();
        // getPhysicalDevice();
        // getInstance();
        std::cout << "releaseResources() " << std::endl;
    }
    void initSwapChainResources() override
    {
        // The following can be used here
        // swapchainImageCount();
        // swapchainImage( index );
        // colorFormat();
        // depthStencilFormat();
        // swapchainImageSize();
        // swapchainImageView();
        std::cout << "initSwapchainResources() " << std::endl;
    }
    void releaseSwapChainResources() override
    {
        std::cout << "releaseSwapChainResources() " << std::endl;
    }
    void render( vkw::Frame &frame) override
    {
        assert( frame.depthImage != VK_NULL_HANDLE);

        frame.clearColor.float32[0] = 0.0f;
        //frame.clearColor.float32[1] = 1.0f;
        //frame.clearColor.float32[2] = 1.0f;
        //frame.clearColor.float32[3] = 1.0f;
        frame.beginRenderPass( frame.commandBuffer );

        frame.endRenderPass(frame.commandBuffer);

        // request the next frame
        // so that this function will be called again
        requestNextFrame();
    }
};

#endif
