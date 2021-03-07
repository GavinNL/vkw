#ifndef VKW_SDLVULKANWINDOW_USAGE_INL
#define VKW_SDLVULKANWINDOW_USAGE_INL

#include "SDLVulkanWindow.h"

#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <cassert>
#include <stdexcept>
#include <set>

namespace vkw
{
inline Frame SDLVulkanWindow::acquireNextFrame()
{
    uint32_t frameIndex;
    vkAcquireNextImageKHR(  m_device,
                            m_swapchain,
                            UINT64_MAX-1,
                            m_imageAvailableSemaphores[0],
                            VK_NULL_HANDLE,
                            &frameIndex);

    vkResetCommandBuffer(m_frames[frameIndex].commandBuffer, 0);

    vkWaitForFences(m_device, 1, &m_fences[frameIndex], VK_FALSE, UINT64_MAX);
    vkResetFences(m_device  , 1, &m_fences[frameIndex]);

    return m_frames[frameIndex];
    //commandBuffer = vulkan->commandBuffers[frameIndex];
    //
    //Frame F;
    //F.swapchainIndex          = frameIndex;
    //F.swapchainImage          = m_swapchainImages[frameIndex];
    //F.swapchainImageView      = m_swapchainImageViews[frameIndex];
    //F.framebuffer             = m_swapchainFrameBuffers[frameIndex];
    //F.commandPool             = m_commandPools[frameIndex];
    //F.imageAvailableSemaphore = m_imageAvailableSemaphores[0];
    //F.renderCompleteSemaphore = m_renderCompleteSemaphores[0];
    //F.clearColor              = {{1.0f, 0.0f, 0.0f, 1.0f}};
    //F.clearDepth              = {1.0f, 0};
}

inline void  SDLVulkanWindow::submitFrame(Frame & C)
{
    submitFrameCommandBuffer(C.commandBuffer, C.imageAvailableSemaphore, C.renderCompleteSemaphore, C.fence);
}

inline void  SDLVulkanWindow::submitFrameCommandBuffer(VkCommandBuffer cb, VkSemaphore wait, VkSemaphore signal, VkFence fence)
{
    VkPipelineStageFlags waitDestStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = &wait;
    submitInfo.pWaitDstStageMask  = &waitDestStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &signal;
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence);
}

inline void SDLVulkanWindow::presentFrame(Frame F)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &F.renderCompleteSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains   = &m_swapchain;
    presentInfo.pImageIndices = &F.swapchainIndex;
    vkQueuePresentKHR(m_presentQueue, &presentInfo);
}

inline void SDLVulkanWindow::waitForPresent()
{
    vkQueueWaitIdle(m_presentQueue);
}
}
#endif
