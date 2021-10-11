#ifndef VKW_VKWVULKANWINDOW_INIT_INL
#define VKW_VKWVULKANWINDOW_INIT_INL

#include "VKWVulkanWindow.h"

#include <vector>
#include <cassert>
#include <stdexcept>
#include <set>
#include <algorithm>
#include <iostream>

namespace vkw
{

Frame VKWVulkanWindow::acquireNextFrame()
{
    if( m_swapchain == VK_NULL_HANDLE)
    {
        _createSwapchain(m_initInfo2.surface.additionalImageCount);
        // level 2 initilization objects
        _createPerFrameObjects();
    }

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
}

void  VKWVulkanWindow::submitFrame(const Frame &C)
{
    submitFrameCommandBuffer(C.commandBuffer, C.imageAvailableSemaphore, C.renderCompleteSemaphore, C.fence);
}

void  VKWVulkanWindow::submitFrameCommandBuffer(VkCommandBuffer cb, VkSemaphore wait, VkSemaphore signal, VkFence fence)
{
    VkPipelineStageFlags waitDestStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSubmitInfo submitInfo         = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &wait;
    submitInfo.pWaitDstStageMask    = &waitDestStageMask;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &cb;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &signal;
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence);
}

void VKWVulkanWindow::presentFrame(Frame const &F)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &F.renderCompleteSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &F.swapchainIndex;
    vkQueuePresentKHR(m_presentQueue, &presentInfo);
}

void VKWVulkanWindow::waitForPresent()
{
    vkQueueWaitIdle(m_presentQueue);
}

void VKWVulkanWindow::setWindowAdapater(VulkanWindowAdapater *window)
{
    m_window = window;
}

std::vector<std::string> VKWVulkanWindow::getAvailableVulkanLayers()
{
    std::vector<std::string> outLayers;

    // Figure out the amount of available layers
    // Layers are used for debugging / validation etc / profiling..
    unsigned int instance_layer_count = 0;
    VkResult res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
    if (res != VK_SUCCESS)
    {
        std::runtime_error("unable to query vulkan instance layer property count");
    }

    std::vector<VkLayerProperties> instance_layer_names(instance_layer_count);
    res = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_names.data());
    if (res != VK_SUCCESS)
    {
        std::runtime_error("unable to retrieve vulkan instance layer names");
    }

    // Display layer names and find the ones we specified above
    std::vector<const char*> valid_instance_layer_names;
    int count(0);
    outLayers.clear();
    for (const auto& name : instance_layer_names)
    {
        outLayers.push_back( name.layerName);
        count++;
    }
    return outLayers;
}

void VKWVulkanWindow::_destroySwapchain(bool destroyRenderPass)
{
    if( m_renderPass != VK_NULL_HANDLE)
    {
        if( destroyRenderPass)
        {
            vkDestroyRenderPass(m_device, m_renderPass, nullptr);
            m_renderPass = VK_NULL_HANDLE;
        }
    }

    if( m_depthStencil != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_device, m_depthStencilImageView, nullptr);
        vkDestroyImage(m_device, m_depthStencil, nullptr);
        vkFreeMemory(m_device,m_depthStencilImageMemory,nullptr);

        m_depthStencil = VK_NULL_HANDLE;
        m_depthStencilImageView = VK_NULL_HANDLE;
        m_depthStencilImageMemory = VK_NULL_HANDLE;
    }

    for(auto & f : m_swapchainFrameBuffers)
    {
        vkDestroyFramebuffer(m_device, f, nullptr);
    }
    m_swapchainFrameBuffers.clear();

    for(auto & f : m_swapchainImageViews)
    {
        vkDestroyImageView(m_device, f, nullptr);
    }
    m_swapchainImageViews.clear();

    m_swapchainImages.clear();

    if( m_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

void VKWVulkanWindow::_createPerFrameObjects()
{
    m_fences.resize( m_swapchainImages.size());
    m_renderCompleteSemaphores.resize( m_swapchainImages.size());
    m_imageAvailableSemaphores.resize( m_swapchainImages.size());

    //vk::CommandPoolCreateInfo cmdC({}, graphics_queue_index);
    m_commandPools.resize( m_swapchainImages.size());
    for(uint32_t i=0;i<m_commandPools.size();i++)
    {
        VkCommandPoolCreateInfo cmdC = {};
        cmdC.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdC.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdC.queueFamilyIndex = static_cast< decltype(cmdC.queueFamilyIndex)>(m_graphicsQueueIndex);

        if( VkResult::VK_SUCCESS != vkCreateCommandPool(m_device, &cmdC, nullptr, &m_commandPools[i]) )
        {
            throw std::runtime_error("Failed to create command pool");
        }
        //===============

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_fences[i]);

        //================

        VkSemaphoreCreateInfo semaphoreCreateInfo = {};// = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_renderCompleteSemaphores[i] );
        vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[i] );
    }


    for(uint32_t i=0;i<m_frames.size();i++)
    {
        Frame & f = m_frames[i];
        f.swapchainIndex = i;
        f.commandPool    = m_commandPools[i];   // the pool it came from;
        //f.commandBuffer = ; // the comman buffer to record
        //f.framebuffer    = m_swapchainFrameBuffers[i];
        //f.renderPass     = m_renderPass;
        //f.swapchainImage = m_swapchainImages[i];
        //f.swapchainImageView = m_swapchainImageViews[i];
        //f.swapchainSize  = m_swapchainSize;
        //f.depthImage     = m_depthStencil;
        //f.depthImageView = m_depthStencilImageView;
        f.imageAvailableSemaphore = m_imageAvailableSemaphores[0]; // the semaphore you have to wait on.
        f.renderCompleteSemaphore = m_renderCompleteSemaphores[0]; // the semaphore you have to trigger when you have
        f.clearColor              = {{1.0f, 1.0f, 1.0f, 1.0f}};
        f.clearDepth              = {1.0f, 0};
        f.fence = m_fences[i];
        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = m_commandPools[i];
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        std::vector<VkCommandBuffer>  commandBuffers;
        commandBuffers.resize(1);
        vkAllocateCommandBuffers(m_device, &allocateInfo, commandBuffers.data());
        f.commandBuffer = commandBuffers[0];

        //m_frames.push_back(f);
    }
}

VKWVulkanWindow::~VKWVulkanWindow()
{
    destroy();
}

void VKWVulkanWindow::destroy()
{
    for(auto & f : m_fences)
    {
        vkDestroyFence(m_device, f, nullptr);
    }
    m_fences.clear();
    for(auto & f : m_renderCompleteSemaphores)
    {
        vkDestroySemaphore(m_device, f, nullptr);
    }
    m_renderCompleteSemaphores.clear();
    for(auto & f : m_imageAvailableSemaphores)
    {
        vkDestroySemaphore(m_device, f, nullptr);
    }
    m_imageAvailableSemaphores.clear();



    for(auto & f : m_commandPools)
    {
        vkDestroyCommandPool(m_device, f, nullptr);
    }
    m_commandPools.clear();

    _destroySwapchain(true);

    if( m_device )
    {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if( m_surface)
    {
        vkDestroySurfaceKHR(m_instance, m_surface,nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if( m_debugCallback )
    {
        auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT"));
        if (func != nullptr)
        {
            func(m_instance, m_debugCallback, nullptr);
        }
        m_debugCallback = nullptr;
    }

    if( m_instance)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    m_window = nullptr;
}

static VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat)
{
        std::vector<VkFormat> depthFormats = {
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM
        };

        for (auto& format : depthFormats)
        {
                VkFormatProperties formatProps;
                vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
                if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                {
                        *depthFormat = format;
                        return true;
                }
        }

        return false;
}

void VKWVulkanWindow::_createFramebuffers()
{
    (void)getSupportedDepthFormat;
    m_swapchainFrameBuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
    {
        std::vector<VkImageView> attachments( m_initInfo2.surface.depthFormat == VK_FORMAT_UNDEFINED? 1 : 2);
        attachments[0] = m_swapchainImageViews[i];

        if( m_initInfo2.surface.depthFormat != VK_FORMAT_UNDEFINED )
            attachments[1] = m_depthStencilImageView;

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width  = m_swapchainSize.width;
        framebufferInfo.height = m_swapchainSize.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFrameBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VKWVulkanWindow::_createRenderPass()
{
    using namespace std;
    vector<VkAttachmentDescription> attachments(1);

        attachments[0].format = m_surfaceFormat.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        if( m_initInfo2.surface.depthFormat != VK_FORMAT_UNDEFINED)
        {
            attachments.resize(2);
            attachments[1].format = getDepthFormat();
            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment =  m_initInfo2.surface.depthFormat != VK_FORMAT_UNDEFINED ? &depthReference : nullptr;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;

        vector<VkSubpassDependency> dependencies(1);

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if(  VkResult::VK_SUCCESS != vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) )
        {
            throw std::runtime_error("Error creating renderpass");
        }
}

void VKWVulkanWindow::_createDepthStencil()
{
    if( getDepthFormat() == VK_FORMAT_UNDEFINED)
        return;

    //VkBool32 validDepthFormat = getSupportedDepthFormat(m_physicalDevice, &m_depthFormat);
    auto p =
    createImage(m_swapchainSize.width, m_swapchainSize.height,
                getDepthFormat(), VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_depthStencil = p.first;
    m_depthStencilImageMemory = p.second;
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = m_depthStencil;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = getDepthFormat();
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthStencilImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swapchain image view!");
        }
    }

}

std::pair<VkImage, VkDeviceMemory> VKWVulkanWindow::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    auto findMemoryType = [this](uint32_t typeFilter, VkMemoryPropertyFlags props)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & props) == props)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    };

    VkImage image;
    VkDeviceMemory imageMemory;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_device, image, imageMemory, 0);

    return {image, imageMemory};
}

VkResult createDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VkDebugReportCallbackEXT VKWVulkanWindow::_createDebug(PFN_vkDebugReportCallbackEXT _callback)
{
    VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo = {};
    debugCallbackCreateInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugCallbackCreateInfo.flags                              = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugCallbackCreateInfo.pfnCallback                        = _callback;

    //SDL2_vkCreateDebugReportCallbackEXT(m_instance, &debugCallbackCreateInfo, 0, &m_debugCallback);

    VkDebugReportCallbackEXT outCallback = VK_NULL_HANDLE;
    if (createDebugReportCallbackEXT(m_instance, &debugCallbackCreateInfo, nullptr, &outCallback) != VK_SUCCESS)
    {
        throw std::runtime_error("unable to create debug report callback extension");
    }
    return outCallback;
}

void VKWVulkanWindow::_createSwapchain(uint32_t additionalImages=1)
{
    using namespace  std;

    auto physical_devices = m_physicalDevice;
    auto surface = m_surface;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_devices, surface,&m_surfaceCapabilities);

    vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t surfaceFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices, surface,
                                                           &surfaceFormatsCount,
                                                           nullptr);
    surfaceFormats.resize(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices, surface,
                                                           &surfaceFormatsCount,
                                                           surfaceFormats.data());

    m_surfaceFormat.format = VK_FORMAT_UNDEFINED;
    for(auto & sf : surfaceFormats)
    {
        if( sf.format == m_initInfo2.surface.surfaceFormat)
        {
            m_surfaceFormat = sf;
            break;
        }
    }
    if(m_surfaceFormat.format == VK_FORMAT_UNDEFINED)
    {
        throw std::runtime_error("Surface cannot use the requested format");
    }

    auto CLAMP = [](uint32_t v, uint32_t m, uint32_t M)
    {
        return std::max( std::min(v, M), m);
    };

    uint32_t width =0,height = 0;

    m_swapchainSize = m_window->getDrawableSize();
    m_swapchainSize.width  = CLAMP(width,  m_surfaceCapabilities.minImageExtent.width , m_surfaceCapabilities.maxImageExtent.width);
    m_swapchainSize.height = CLAMP(height, m_surfaceCapabilities.minImageExtent.height, m_surfaceCapabilities.maxImageExtent.height);

    m_swapchainSize = m_surfaceCapabilities.currentExtent;
    uint32_t imageCount = m_surfaceCapabilities.minImageCount + additionalImages;
    if (m_surfaceCapabilities.maxImageCount > 0 && imageCount > m_surfaceCapabilities.maxImageCount)
    {
        imageCount = m_surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = m_surfaceFormat.format;
    createInfo.imageColorSpace  = m_surfaceFormat.colorSpace;
    createInfo.imageExtent      = m_swapchainSize;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(m_graphicsQueueIndex), static_cast<uint32_t>(m_presentQueueIndex) };
    if (m_graphicsQueueIndex != m_presentQueueIndex)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform   = m_surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = m_initInfo2.surface.presentMode;
    createInfo.clipped        = VK_TRUE;

    if(VkResult::VK_SUCCESS != vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) )
    {
        throw std::runtime_error("Failed to create swapchain");
    }

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr);
    m_swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, m_swapchainImages.data());


    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (uint32_t i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_surfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swapchain image view!");
        }
    }


    _createDepthStencil();
    if( m_renderPass == VK_NULL_HANDLE)
        _createRenderPass();
    _createFramebuffers();

    if(m_frames.size() == 0)
        m_frames.resize( m_swapchainImages.size() );

    for(uint32_t i=0;i<m_swapchainImages.size();i++)
    {
        Frame & f = m_frames[i];

        f.swapchainIndex = i;

        f.framebuffer    = m_swapchainFrameBuffers[i];
        f.renderPass     = m_renderPass;
        f.swapchainImage = m_swapchainImages[i];
        f.swapchainImageView = m_swapchainImageViews[i];
        f.swapchainSize  = m_swapchainSize;
        f.depthImage     = m_depthStencil;
        f.depthImageView = m_depthStencilImageView;
        f.depthFormat    = m_initInfo2.surface.depthFormat;
        f.swapchainFormat= m_surfaceFormat.format;

    }
}

void VKWVulkanWindow::_selectQueueFamily()
{
    auto physical_devices = m_physicalDevice;
    auto surface          = m_surface;

    using namespace std;
    vector<VkQueueFamilyProperties> queueFamilyProperties;
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices, &queueFamilyCount, nullptr);
    queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices, &queueFamilyCount, queueFamilyProperties.data());

    int graphicIndex = -1;
    int presentIndex = -1;

    int i = 0;
    for(const auto& queueFamily : queueFamilyProperties)
    {
        if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicIndex = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices, static_cast<uint32_t>(i), surface, &presentSupport);
        if(queueFamily.queueCount > 0 && presentSupport)
        {
            presentIndex = i;
        }

        if(graphicIndex != -1 && presentIndex != -1)
        {
            break;
        }

        i++;
    }

    m_graphicsQueueIndex = graphicIndex;
    m_presentQueueIndex = presentIndex;
}

std::vector<std::string> _validateExtension(std::vector<std::string> const & ext, std::set<std::string> const & valid)
{
    std::vector<std::string> out;
    for(auto & e : ext)
    {
        if( valid.count(e) )
        {
            out.push_back(e);
        }
        else
        {
            std::cerr << "Invalid Extension: " << e << std::endl;
        }
    }
    std::sort( out.begin(), out.end() );

    auto it = std::unique(out.begin(), out.end() );
    out.erase( it, out.end() );
    return out;
}

void VKWVulkanWindow::createVulkanInstance(InstanceInitilizationInfo2 const & I)
{
    using namespace std;

    assert(m_window);

    //=================================================================
    m_initInfo2.instance = I;

    //=================================================================
    // Make sure there are no duplicate layers
    //=================================================================
    vectorAppend(m_initInfo2.instance.enabledExtensions, m_window->getRequiredVulkanExtensions());

    vectorUnique(m_initInfo2.instance.enabledLayers);
    vectorUnique(m_initInfo2.instance.enabledExtensions);

    m_initInfo2.instance.enabledLayers = _validateExtension(m_initInfo2.instance.enabledLayers,
                                                                getSupportedLayers());

    m_initInfo2.instance.enabledExtensions = _validateExtension(m_initInfo2.instance.enabledExtensions,
                                                                getSupportedInstanceExtensions());
    //=================================================================


    {
        std::vector<const char *> validationLayers;
        std::vector<const char *> extensionNames;

        for(auto & x : m_initInfo2.instance.enabledLayers)
        {
            std::cerr << "Enabling Instance Layer: " << x << std::endl;
            validationLayers.push_back(x.data());
        }

        for(auto & x : m_initInfo2.instance.enabledExtensions)
        {
            std::cerr << "Enabling Extension: " << x << std::endl;
            extensionNames.push_back(x.data());
        }


        VkApplicationInfo appInfo = {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = m_initInfo2.instance.applicationName.data();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = m_initInfo2.instance.engineName.data();
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = m_initInfo2.instance.vulkanVersion;

        VkInstanceCreateInfo instanceCreateInfo    = {};
        instanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo        = &appInfo;
        instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames     = validationLayers.data();
        instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensionNames.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

        VkInstance instance;
        if( VkResult::VK_SUCCESS != vkCreateInstance(&instanceCreateInfo, nullptr, &instance) )
        {
            throw std::runtime_error("Failed to create Vulkan Instance");
        }
        m_instance = instance;

        if( m_initInfo2.instance.debugCallback )
        {
            m_debugCallback = _createDebug(m_initInfo2.instance.debugCallback);
        }
    }


}

bool VKWVulkanWindow::createVulkanSurface(SurfaceInitilizationInfo2 const & I)
{
    m_initInfo2.surface = I;
    m_surface = m_window->createSurface(m_instance);
    return m_surface != VK_NULL_HANDLE;
}

VkPhysicalDeviceFeatures2 VKWVulkanWindow::getSupportedDeviceFeatures(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceFeatures2 availableDeviceFeatures2 = {};

    availableDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    availableDeviceFeatures2.pNext = nullptr;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &availableDeviceFeatures2);

    return availableDeviceFeatures2;
}

VkPhysicalDeviceVulkan11Features VKWVulkanWindow::getSupportedDeviceFeatures11(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceVulkan11Features v11 = {};
    v11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    VkPhysicalDeviceFeatures2 availableDeviceFeatures2 = {};
    availableDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    availableDeviceFeatures2.pNext = &v11;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &availableDeviceFeatures2);
    return v11;
}
VkPhysicalDeviceVulkan12Features VKWVulkanWindow::getSupportedDeviceFeatures12(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceVulkan12Features v12 = {};
    v12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceFeatures2 availableDeviceFeatures2 = {};
    availableDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    availableDeviceFeatures2.pNext = &v12;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &availableDeviceFeatures2);
    return v12;
}

void VKWVulkanWindow::createVulkanDevice(const DeviceInitilizationInfo2 &I)
{
    {
        if( m_initInfo2.device.deviceID == 0)
        {
            m_physicalDevice =  chooseVulkanPhysicalDevice([](auto & props)
            {
                return props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            });

            if( m_physicalDevice == VK_NULL_HANDLE)
            {
                m_physicalDevice = chooseVulkanPhysicalDevice([](auto & props)
                {
                    (void)props;
                    return true;
                });
            }
        }
        else
        {
            m_physicalDevice = chooseVulkanPhysicalDevice([&](auto & props)
            {
                return props.deviceID == I.deviceID;
            });

        }
    }
    if(  m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Could not find a proper physical device");
    }
    m_initInfo2.device = I;
    // find the proper queue indices
    _selectQueueFamily();
    //==========
    std::vector<const char*> deviceExtensions;
    {
        auto supportedExtensions = getSupportedDeviceExtensions(m_physicalDevice);
        m_initInfo2.device.deviceExtensions = _validateExtension(m_initInfo2.device.deviceExtensions,
                                                                 supportedExtensions);

        for(auto & e : m_initInfo2.device.deviceExtensions)
        {
            std::cerr << "Enabling Device Extension: " << e << std::endl;
            deviceExtensions.push_back(e.data());
        }
    }

    const float queue_priority[] = { 1.0f };

    assert(m_graphicsQueueIndex >= 0);
    assert(m_presentQueueIndex >= 0);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { static_cast<uint32_t>(m_graphicsQueueIndex), static_cast<uint32_t>(m_presentQueueIndex) };

    float queuePriority = queue_priority[0];
    for(auto queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex        = queueFamily;
        queueCreateInfo.queueCount              = 1;
        queueCreateInfo.pQueuePriorities        = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //==============================================================================
    // Double check that all the device features which have been requested
    // are supported. If there are any that are not supported, turn them off
    //==============================================================================
    {
        // v1.0
        {
            auto sup10 = getSupportedDeviceFeatures(m_physicalDevice);

            VkBool32 *avilFeat      = &sup10.features.robustBufferAccess;
            VkBool32 *availFeatEnd  = &sup10.features.inheritedQueries;
            VkBool32 *requestedFeat = &m_initInfo2.device.enabledFeatures.features.robustBufferAccess;

            while( avilFeat != availFeatEnd)
            {
                *requestedFeat++ &= *avilFeat++;
            }
        }

        {
            auto sup11 = getSupportedDeviceFeatures11(m_physicalDevice);
            // v1.1
            VkBool32 *avilFeat      = &sup11.storageBuffer16BitAccess;
            VkBool32 *availFeatEnd  = &sup11.shaderDrawParameters;
            VkBool32 *requestedFeat = &m_initInfo2.device.enabledFeatures11.storageBuffer16BitAccess;

            while( avilFeat != availFeatEnd)
            {
                *requestedFeat++ &= *avilFeat++;
            }
        }

        {
            auto sup12 = getSupportedDeviceFeatures12(m_physicalDevice);
            // v1.1
            VkBool32 *avilFeat      = &sup12.samplerMirrorClampToEdge;
            VkBool32 *availFeatEnd  = &sup12.subgroupBroadcastDynamicId;
            VkBool32 *requestedFeat = &m_initInfo2.device.enabledFeatures12.samplerMirrorClampToEdge;

            while( avilFeat != availFeatEnd)
            {
                *requestedFeat++ &= *avilFeat++;
            }
        }
    }
    //==============================================================================

    m_initInfo2.device.enabledFeatures.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    m_initInfo2.device.enabledFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_initInfo2.device.enabledFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    m_initInfo2.device.enabledFeatures.pNext   = &m_initInfo2.device.enabledFeatures11;
    m_initInfo2.device.enabledFeatures11.pNext = &m_initInfo2.device.enabledFeatures12;

    //https://en.wikipedia.org/wiki/Anisotropic_filtering
    //VkPhysicalDeviceFeatures deviceFeatures = {};
    //deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();

    // use the extended method, see below
    createInfo.pEnabledFeatures        = nullptr;

    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    std::vector<const char*> validationLayers;//
    for(auto &x : m_initInfo2.instance.enabledLayers)
        validationLayers.push_back(x.data());

    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    //===================================================
    // enable features using the extended method
    m_initInfo2.device.enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    createInfo.pNext = &m_initInfo2.device.enabledFeatures;
    //===================================================

    if( VkResult::VK_SUCCESS != vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) )
    {
        throw std::runtime_error("Failed to create device");
    }

    vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_graphicsQueueIndex), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_presentQueueIndex ), 0, &m_presentQueue);

    if( m_swapchain == VK_NULL_HANDLE)
    {
        _createSwapchain(m_initInfo2.surface.additionalImageCount);
        // level 2 initilization objects
        _createPerFrameObjects();
    }
}


}


#endif
