#ifndef QTSDL_VULKAN_FRAME_H
#define QTSDL_VULKAN_FRAME_H

#include <vulkan/vulkan.h>

struct Frame
{
    uint32_t         swapchainIndex;
    VkCommandBuffer  commandBuffer; // the command buffer to record. THis buffer is automatically reset
                                    //  when acquireFrame() is called. If you need more command buffers
                                    //  you can allocate it from the commandPool. You will need
                                    //  to manage these extra buffers yourself.

    VkCommandPool    commandPool;   // The command pool for this frame. Do not RESET this pool
    VkFramebuffer    framebuffer;
    VkRenderPass     renderPass;
    VkImage          swapchainImage;
    VkImageView      swapchainImageView;
    VkFormat         swapchainFormat;

    VkExtent2D       swapchainSize;
    VkImage          depthImage;
    VkImageView      depthImageView;
    VkFormat         depthFormat;

    VkSemaphore      imageAvailableSemaphore; // the semaphore you have to wait on.
    VkSemaphore      renderCompleteSemaphore; // the semaphore you have to trigger when you have
                                              //   submitted your last command buffer.

    VkFence          fence;                   // the fence you should trigger when submitting this frame

    VkClearColorValue        clearColor;
    VkClearDepthStencilValue clearDepth;

    void beginCommandBuffer()
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
    }
    void endCommandBuffer()
    {
        vkEndCommandBuffer(commandBuffer);
    }

    void beginRenderPass( VkCommandBuffer cmd )
    {
        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass        = renderPass;
        render_pass_info.framebuffer       = framebuffer;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapchainSize;
        render_pass_info.clearValueCount   = 1;

        std::vector<VkClearValue> clearValues(2);
        clearValues[0].color = clearColor;
        clearValues[1].depthStencil = clearDepth;

        render_pass_info.clearValueCount = depthImage==VK_NULL_HANDLE? 1 : static_cast<uint32_t>(clearValues.size());
        render_pass_info.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    }
    void endRenderPass(VkCommandBuffer cmd)
    {
        vkCmdEndRenderPass(cmd);
    }
};


#endif
