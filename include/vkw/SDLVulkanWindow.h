#ifndef VKW_SDL_VULKAN_WINDOW_H
#define VKW_SDL_VULKAN_WINDOW_H

#if __has_include(<SDL2/SDL2.h>)
#include <SDL2/SDL2.h>
#include <SDL2/SDL_vulkan.h>
#else
#include <SDL.h>
#include <SDL_vulkan.h>
#endif

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include "Frame.h"
#include "base_widget.h"

namespace vkw
{
class SDLVulkanWindow : public BaseWidget
{
    public:

    struct InstanceInitilizationInfo2
    {
        PFN_vkDebugReportCallbackEXT debugCallback = nullptr;
        std::vector<std::string> enabledLayers     = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation"};
        std::vector<std::string> enabledExtensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };

        #if defined VK_HEADER_VERSION_COMPLETE
            #define VKW_DEFAULT_VULKAN_VERSION VK_HEADER_VERSION_COMPLETE
        #else
            #define VKW_DEFAULT_VULKAN_VERSION VK_MAKE_VERSION(1, 0, 0)
        #endif
        uint32_t                 vulkanVersion     = VKW_DEFAULT_VULKAN_VERSION;
    };


    struct SurfaceInitilizationInfo2
    {
        VkFormat         depthFormat          = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
        VkPresentModeKHR presentMode          = VK_PRESENT_MODE_FIFO_KHR;
        uint32_t         additionalImageCount = 1;// how many additional swapchain images should we create ( total = min_images + additionalImageCount
    };

    struct DeviceInitilizationInfo2
    {
        std::vector<std::string>         deviceExtensions  = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};


        // the values enabledFeaturesXX structs can be set to true
        // to enable the features. you do not need to set the sNext
        // pointers. They will automatically be set when we create the device

        // if you want enable non KHR extensions, then you will have to set
        // the enabledFeatures12.pNext value yourself
        VkPhysicalDeviceFeatures2        enabledFeatures   = {};
        VkPhysicalDeviceVulkan11Features enabledFeatures11 = {};
        VkPhysicalDeviceVulkan12Features enabledFeatures12 = {};

    };

    struct
    {
        InstanceInitilizationInfo2 instance;
        SurfaceInitilizationInfo2  surface;
        DeviceInitilizationInfo2   device;
    } m_initInfo2;

    void createVulkanInstance(InstanceInitilizationInfo2 const & I);
    void createVulkanSurface(SurfaceInitilizationInfo2 const & I);
    void createPhysicalDevice();
    void createVulkanDevice(DeviceInitilizationInfo2 const & I);

    // 1. Create the  window first using this function
    void createWindow(const char *title, int x, int y, int w, int h, Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);


    std::vector<std::string> getAvailableVulkanExtensions();
    std::vector<std::string> getAvailableVulkanLayers();


    ~SDLVulkanWindow();

    void destroy();

    //===================================================================
    // The implementation for the following functions are in
    //  SDLVulkanWindow_USAGE.cpp
    //===================================================================
    Frame acquireNextFrame();

    void  submitFrameCommandBuffer(VkCommandBuffer cb, VkSemaphore wait, VkSemaphore signal, VkFence fence);
    void  submitFrame(Frame & C);
    void  presentFrame(Frame F);
    void  waitForPresent();

    SDL_Window* getSDLWindow() const
    {
        return m_window;
    }

    VkInstance getInstance() const
    {
        return m_instance;
    }
    VkExtent2D getSwapchainExtent() const
    {
        return m_swapchainSize;
    }
    VkDevice getDevice() const
    {
        return m_device;
    }
    VkPhysicalDevice getPhysicalDevice() const
    {
        return m_physicalDevice;
    }
    VkSwapchainKHR getSwapchain() const
    {
        return m_swapchain;
    }
    int32_t getGraphicsQueueIndex() const
    {
        return m_graphicsQueueIndex;
    }
    VkQueue getGraphicsQueue() const
    {
        return m_graphicsQueue;
    }
    int32_t getPresentQueueIndex() const
    {
        return m_presentQueueIndex;
    }
    VkQueue getPresentQueue() const
    {
        return m_presentQueue;
    }
    std::vector<VkImageView> getSwapchainImageViews() const
    {
        return m_swapchainImageViews;
    }
    std::vector<VkImage> getSwapchainImages() const
    {
        return m_swapchainImages;
    }
    VkFormat getSwapchainFormat() const
    {
        return m_surfaceFormat.format;
    }
    VkFormat getDepthFormat() const
    {
        return m_initInfo2.surface.depthFormat;
    }
    VkImage getDepthImage() const
    {
        return m_depthStencil;
    }
    VkImageView getDepthImageView() const
    {
        return m_depthStencilImageView;
    }
    //===================================================================

    void rebuildSwapchain()
    {
        _destroySwapchain(false);
        _createSwapchain(m_additionalImages);
    }

    static VkPhysicalDeviceFeatures2 getSupportedDeviceFeatures(VkPhysicalDevice physicalDevice);
    static VkPhysicalDeviceVulkan11Features getSupportedDeviceFeatures11(VkPhysicalDevice physicalDevice);
    static VkPhysicalDeviceVulkan12Features getSupportedDeviceFeatures12(VkPhysicalDevice physicalDevice);
protected:
    SDL_Window *             m_window   = nullptr;
    VkInstance               m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface  = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physicalDevice;

    int32_t  m_graphicsQueueIndex;
    int32_t  m_presentQueueIndex;
    VkDevice m_device = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue  = VK_NULL_HANDLE;

    VkSurfaceCapabilitiesKHR m_surfaceCapabilities;
    VkSurfaceFormatKHR       m_surfaceFormat;
    VkExtent2D               m_swapchainSize;
    uint32_t                 m_additionalImages = 0;
    VkSwapchainKHR           m_swapchain        = VK_NULL_HANDLE;
    //VkPresentModeKHR         m_presentMode;

    std::vector<VkImage>     m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    //VkFormat       m_depthFormat             = VK_FORMAT_UNDEFINED;
    VkImage        m_depthStencil            = VK_NULL_HANDLE;
    VkImageView    m_depthStencilImageView   = VK_NULL_HANDLE;
    VkDeviceMemory m_depthStencilImageMemory = VK_NULL_HANDLE;

    VkRenderPass               m_renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_swapchainFrameBuffers;
    std::vector<VkCommandPool> m_commandPools;

    std::vector<VkFence>     m_fences;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderCompleteSemaphores;

    VkDebugReportCallbackEXT m_debugCallback = VK_NULL_HANDLE;

    std::vector<Frame> m_frames;

private:
    SDL_Window *     _createWindow();

    void             _selectQueueFamily();
    VkDevice         _createDevice();
    void             _createSwapchain(uint32_t additionalImages);
    void             _destroySwapchain(bool destroyRenderpass);
    VkDebugReportCallbackEXT _createDebug(PFN_vkDebugReportCallbackEXT _callback);

    std::pair<VkImage, VkDeviceMemory> createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    //const std::vector<const char*> validationLayers = {
    //    ///has bug
    //    "VK_LAYER_LUNARG_standard_validation"
    //};

    void _createDepthStencil();
    void _createRenderPass();
    void _createFramebuffers();

    void _createPerFrameObjects();
};
}

#include "SDLVulkanWindow_INIT.inl"
#include "SDLVulkanWindow_USAGE.inl"
#endif
