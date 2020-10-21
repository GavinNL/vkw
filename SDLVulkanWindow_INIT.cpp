#include "SDLVulkanWindow.h"

#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <cassert>
#include <stdexcept>
#include <set>

void SDLVulkanWindow::createVulkanInstance(InitilizationInfo info)
{
    m_initInfo = info;
    m_instance = _createInstance();
    _createDebug();
}

void SDLVulkanWindow::createWindow(const char *title, int x, int y, int w,
                                                      int h, Uint32 flags)
{
    flags |= SDL_WINDOW_VULKAN;
    m_window = SDL_CreateWindow( title, x, y,w,h, flags);
}

std::vector<std::string> SDLVulkanWindow::getAvailableVulkanExtensions()
{
    std::vector<std::string> outExtensions;
    // Figure out the amount of extensions vulkan needs to interface with the os windowing system
    // This is necessary because vulkan is a platform agnostic API and needs to know how to interface with the windowing system
    unsigned int ext_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(m_window, &ext_count, nullptr))
    {
        std::runtime_error("Unable to query the number of Vulkan instance extensions");
        return {};
    }

    // Use the amount of extensions queried before to retrieve the names of the extensions
    std::vector<const char*> ext_names(ext_count);
    if (!SDL_Vulkan_GetInstanceExtensions(m_window, &ext_count, ext_names.data()))
    {
        std::runtime_error("Unable to query the number of Vulkan instance extension names");
    }

    // Display names
    //std::runtime_error("found " << ext_count << " Vulkan instance extensions:\n";
    for (unsigned int i = 0; i < ext_count; i++)
    {
        //std::cout << i << ": " << ext_names[i] << "\n";
        outExtensions.emplace_back(ext_names[i]);
    }

    // Add debug display extension, we need this to relay debug messages
    outExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    return outExtensions;
}

std::vector<std::string> SDLVulkanWindow::getAvailableVulkanLayers()
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
    //const std::set<std::string>& lookup_layers = getRequestedLayerNames();
    int count(0);
    outLayers.clear();
    for (const auto& name : instance_layer_names)
    {
        outLayers.push_back( name.layerName);
        count++;
    }

    // Print the ones we're enabling
    //std::cout << "\n";
    //for (const auto& layer : outLayers)
    //    std::cout << "applying layer: " << layer.c_str() << "\n";

    return outLayers;
}

void SDLVulkanWindow::_destroySwapchain()
{
    if( m_renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    if( m_depthStencil != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_device, m_depthStencilImageView, nullptr);
        vkDestroyImage(m_device, m_depthStencil, nullptr);
        vkFreeMemory(m_device,m_depthStencilImageMemory,nullptr);

        m_depthFormat = VK_FORMAT_UNDEFINED;
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

void SDLVulkanWindow::initSurface( SDLVulkanWindow::SurfaceInitilizationInfo I)
{
    m_additionalImages = I.additionalImageCount;
    m_depthFormat = I.depthFormat;
    m_presentMode = I.presentMode;
    m_physicalDeviceFeatures = I.enabledFeatures;

    //_createDebug();
    SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);

    m_physicalDevice = _selectPhysicalDevice();
    _selectQueueFamily();
    m_device = _createDevice();

    _createSwapchain(I.additionalImageCount);

    // level 2 initilization objects
    _createPerFrameObjects();
}

//void SDLVulkanWindow::init()
//{
//    //m_window = _createWindow();
//    //m_instance = _createInstance();
//    _createDebug();
//    SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);
//    m_physicalDevice = _selectPhysicalDevice();
//    _selectQueueFamily();
//    m_device = _createDevice();


//    _createSwapchain(1);

//    // level 2 initilization objects
//    _createPerFrameObjects();
//}

void SDLVulkanWindow::_createPerFrameObjects()
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

SDLVulkanWindow::~SDLVulkanWindow()
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

    _destroySwapchain();

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
        auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr)
        {
            func(m_instance, m_debugCallback, nullptr);
        }
    }

    if( m_instance)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
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

void SDLVulkanWindow::_createFramebuffers()
{
    (void)getSupportedDepthFormat;
    m_swapchainFrameBuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
    {
        std::vector<VkImageView> attachments( m_depthFormat==VK_FORMAT_UNDEFINED? 1 : 2);
        attachments[0] = m_swapchainImageViews[i];

        if( m_depthFormat != VK_FORMAT_UNDEFINED )
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

void SDLVulkanWindow::_createRenderPass()
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

        if( m_depthFormat != VK_FORMAT_UNDEFINED)
        {
            attachments.resize(2);
            attachments[1].format = m_depthFormat;
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
        subpassDescription.pDepthStencilAttachment =  m_depthFormat != VK_FORMAT_UNDEFINED ? &depthReference : nullptr;
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

void SDLVulkanWindow::_createDepthStencil()
{
    if( m_depthFormat == VK_FORMAT_UNDEFINED)
        return;

    //VkBool32 validDepthFormat = getSupportedDepthFormat(m_physicalDevice, &m_depthFormat);
    auto p =
    createImage(m_swapchainSize.width, m_swapchainSize.height,
                m_depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_depthStencil = p.first;
    m_depthStencilImageMemory = p.second;
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_depthStencil;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthStencilImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swapchain image view!");
        }
    }
}

std::pair<VkImage, VkDeviceMemory> SDLVulkanWindow::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
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
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void SDLVulkanWindow::_createDebug()
{
    //PFN_vkCreateDebugReportCallbackEXT SDL2_vkCreateDebugReportCallbackEXT = nullptr;
    //SDL2_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)SDL_Vulkan_GetVkGetInstanceProcAddr();

    if( m_initInfo.callback)
    {
        VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo = {};
        debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        debugCallbackCreateInfo.pfnCallback = m_initInfo.callback;

        //SDL2_vkCreateDebugReportCallbackEXT(m_instance, &debugCallbackCreateInfo, 0, &m_debugCallback);

        if (createDebugReportCallbackEXT(m_instance, &debugCallbackCreateInfo, nullptr, &m_debugCallback) != VK_SUCCESS)
        {
            throw std::runtime_error("unable to create debug report callback extension");
        }
    }
}

void SDLVulkanWindow::_createSwapchain(uint32_t additionalImages=1)
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

    if(surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM)
        {
        throw std::runtime_error("surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM");
        }

    auto CLAMP = [](uint32_t v, uint32_t m, uint32_t M)
    {
        return std::max( std::min(v, M), m);
    };
    m_surfaceFormat = surfaceFormats[0];
    uint32_t width,height = 0;
    int32_t  iwidth,iheight = 0;
    SDL_Vulkan_GetDrawableSize(m_window, &iwidth, &iheight);
    width = static_cast<uint32_t>(iwidth);
    height = static_cast<uint32_t>(iheight);
    width  = CLAMP(width,  m_surfaceCapabilities.minImageExtent.width , m_surfaceCapabilities.maxImageExtent.width);
    height = CLAMP(height, m_surfaceCapabilities.minImageExtent.height, m_surfaceCapabilities.maxImageExtent.height);
    m_swapchainSize.width  = width;
    m_swapchainSize.height = height;

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
    createInfo.presentMode    = m_presentMode;
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
        f.depthFormat    = m_depthFormat;
        f.swapchainFormat= m_surfaceFormat.format;

    }
}

VkDevice SDLVulkanWindow::_createDevice()
{
    using namespace std;

    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const float queue_priority[] = { 1.0f };

    assert(m_graphicsQueueIndex >= 0);
    assert(m_presentQueueIndex >= 0);

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    set<uint32_t> uniqueQueueFamilies = { static_cast<uint32_t>(m_graphicsQueueIndex), static_cast<uint32_t>(m_presentQueueIndex) };

    float queuePriority = queue_priority[0];
    for(auto queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(m_graphicsQueueIndex);
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &deviceFeatures);
    VkBool32 * feat = reinterpret_cast<VkBool32*>(&deviceFeatures);
    VkBool32 * featEnd = feat + sizeof(deviceFeatures)/sizeof(VkBool32);
    uint32_t i=0;
    while( feat != featEnd)
    {
        *feat &= reinterpret_cast<VkBool32*>(&m_physicalDeviceFeatures)[i++];
        feat++;
    }
    //https://en.wikipedia.org/wiki/Anisotropic_filtering
    //VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    std::vector<const char*> validationLayers;//
    for(auto &x : m_initInfo.enabledLayers)
        validationLayers.push_back(x.data());

    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    if( VkResult::VK_SUCCESS != vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) )
    {
        throw std::runtime_error("Failed to create device");
    }

    vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_graphicsQueueIndex), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_presentQueueIndex ), 0, &m_presentQueue);
    return  m_device;
}

void SDLVulkanWindow::_selectQueueFamily()
{
    auto physical_devices = m_physicalDevice;
    auto surface = m_surface;

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

VkPhysicalDevice SDLVulkanWindow::_selectPhysicalDevice()
{
    using namespace std;
    vector<VkPhysicalDevice> physicalDevices;
    uint32_t physicalDeviceCount = 0;

    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
    physicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());



    //m_physicalDevice = physicalDevices[0];
    return physicalDevices[0];;
}

VkInstance SDLVulkanWindow::_createInstance()
{
    using namespace std;

    assert(m_window);

    auto extNames = getAvailableVulkanExtensions();

   // vector<const char *> validationLayers;
    vector<const char *> extensionNames;
    for(auto & e : extNames) extensionNames.push_back( e.data() );


    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "App name";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vector<const char*> validationLayers;
    for(auto & x : m_initInfo.enabledLayers)
        validationLayers.push_back(x.data());

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

    VkInstance instance;
    if( VkResult::VK_SUCCESS != vkCreateInstance(&instanceCreateInfo, nullptr, &instance) )
    {
        throw std::runtime_error("Failed to create Vulkan Instance");
    }
    return instance;
}

