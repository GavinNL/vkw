#ifndef VKW_SDLQT_VULKANAPPLICATION_H
#define VKW_SDLQT_VULKANAPPLICATION_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "Frame.h"

namespace vkw
{
class Application
{
public:


    virtual ~Application()
    {
    }
    /**
     * @brief init
     * @param System
     *
     * This function will be called to initilize
     * the all the memory/objects you need.
     *
     * This should basically be used as your constructor.
     *
     * The swapchain may not have been created by this point
     *
     */
    virtual void initResources() = 0;

    /**
     * @brief releaseResources
     *
     * This is called when the vulkan application is about to be shut down
     * Use this to release all vulkan resources.
     */
    virtual void releaseResources() = 0;


    /**
     * @brief initSwapChainResources
     *
     *
     * This method is called whenever the swapchain changes its size.
     * we can use this method to allocate any offscreen render targets.
     * that might be dependent on the swapchain size.
     */
    virtual void initSwapChainResources() = 0;


    /**
     * @brief releaseSwapChainResources
     *
     * This method gets called whenever the swapchain has been
     * resized. This method will be called to release any
     * memory or resources which was allocated by a previous call to
     * initSwapChainResources()
     *
     * After this method is called, another call to initSwapChainResources()
     * will automatically be called.
     */
    virtual void releaseSwapChainResources() = 0;



    /**
     * @brief preRender
     *
     * Called prior to rendering the frame. You can use this method
     * to update any descriptor sets that will be used for the n
     * next frame.
     */
    virtual void preRender() {};


    /**
     * @brief render
     * @param frame
     *
     * The render() method is called at each frame and at
     * a rate determiend by the RenderSurface.
     *
     * frame contains the following information which you can use
     * : frame.
     *
     *   frame.defaultRenderPass - the default render pass
     *   frame.currentFrameBuffer - the current framebuffer in the render pass;
     *   frame.currentCommandBuffer - the command buffer to be used to draw;
     *   frame.swapChainImageSize - the extents of the swapchain;
     */
    virtual void render(Frame &frame) = 0;


    virtual void nativeWindowEvent(void const * e)
    {
        (void)e;
    }
    virtual void postRender()  {};
    //=========================================================================



    void requestNextFrame()
    {
        renderNextFrame();
    }
    void renderNextFrame()
    {
        m_renderNextFrame=true;
    }
    bool shouldRender() const
    {
        return m_renderNextFrame;
    }

    //=========================================================================
    VkExtent2D swapchainImageSize() const
    {
        return m_swapChainSize;
    }

    VkFormat colorFormat() const
    {
        return m_swapChainFormat;
    }
    VkFormat depthStencilFormat() const
    {
        return m_swapChainDepthFormat;
    }
    uint32_t swapchainImageCount() const
    {
        return static_cast<uint32_t>(m_swapchainImages.size());
    }
    uint32_t currentSwapchainImageIndex() const
    {
        return m_currentSwapchainIndex;
    }
    VkImage swapchainImage(uint32_t indx) const
    {
        return m_swapchainImages.at(indx);
    }
    VkImageView swapchainImageView(uint32_t indx) const
    {
        return m_swapchainImageViews.at(indx);
    }
    VkDevice getDevice() const
    {
        return m_device;
    }
    VkPhysicalDevice getPhysicalDevice() const
    {
        return m_physicalDevice;
    }
    VkInstance getInstance() const
    {
        return m_instance;
    }

    //=========================================================================


    uint32_t concurrentFrameCount() const
    {
        return m_concurrentFrameCount;
    }

    VkRenderPass getDefaultRenderPass() const
    {
        return m_defaultRenderPass;
    }

    void quit()
    {
        m_quit=true;
    }

    bool shouldQuit() const
    {
        return m_quit;
    }
protected:
    friend class SDLVulkanWidget3;


    VkInstance               m_instance;
    VkDevice                 m_device;
    VkPhysicalDevice         m_physicalDevice;

    VkExtent2D               m_swapChainSize;
    VkFormat                 m_swapChainFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat                 m_swapChainDepthFormat= VkFormat::VK_FORMAT_UNDEFINED;
    uint32_t                 m_concurrentFrameCount=0;
    uint32_t                 m_currentSwapchainIndex=0;
    std::vector<VkImage>     m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    VkRenderPass             m_defaultRenderPass;
    bool                     m_quit=false;
    bool                     m_renderNextFrame=true;

    friend class QTVulkanWidget;
    friend class SDLVulkanWidget;
    friend class SDLVulkanWidget2;
    friend class QtVulkanWidget2;
    friend class SDLVulkanWidget3;
    friend class QTRenderer;

};
}

#endif
