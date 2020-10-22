#ifndef VWK_MY_TEST_APPLICATION_H
#define VWK_MY_TEST_APPLICATION_H


#include <vkw/VulkanApplication.h>
#include <iostream>

class MyApplication : public Application
{
    // Application interface
public:
    void initResources() override
    {
        std::cout << "ir " << std::endl;
    }
    void releaseResources() override
    {
        std::cout << "rr " << std::endl;
    }
    void initSwapChainResources() override
    {
        std::cout << "is " << std::endl;
    }
    void releaseSwapChainResources() override
    {
        std::cout << "rs " << std::endl;
    }
    void render(Frame &frame) override
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
