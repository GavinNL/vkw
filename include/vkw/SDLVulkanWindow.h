#ifndef SDL_VULKAN_WINDOW_H
#define SDL_VULKAN_WINDOW_H

#include <vulkan/vulkan.h>

#include<SDL2/SDL.h>
#include <vector>
#include <string>
#include "Frame.h"

namespace vkw
{
class SDLVulkanWindow
{
    public:

    // Used to initialize the vulkan instance/devices
    struct InitilizationInfo
    {
        PFN_vkDebugReportCallbackEXT callback = nullptr;
        std::vector<std::string> enabledLayers     = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation"};
        std::vector<std::string> enabledExtensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
    };

    struct SurfaceInitilizationInfo
    {
        VkFormat           depthFormat = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
        VkPresentModeKHR   presentMode = VK_PRESENT_MODE_FIFO_KHR;
        uint32_t  additionalImageCount = 1; // how many additional swapchain images should we create ( total = min_images + additionalImageCount
        VkPhysicalDeviceFeatures enabledFeatures = {}; // which additional features should we enable?
                                                  // if the device does not support this feature, it will not be enabled
    };


    // 1. Create the  window first using this function
    void createWindow(const char *title, int x, int y, int w, int h, Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

    // 2. Initialize the vulkan instance by calling this.
    //    Use the default Initilization struct if you do not need anything extra
    void createVulkanInstance(InitilizationInfo info);

    // 3. Call this function to create the device and all other primitives
    void initSurface(SDLVulkanWindow::SurfaceInitilizationInfo I);


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
    VkQueue getGraphicsQueue() const
    {
        return m_graphicsQueue;
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
        return m_depthFormat;
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

protected:


        InitilizationInfo m_initInfo;
        SDL_Window * m_window = nullptr;
        VkInstance   m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface  = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice;
        VkPhysicalDeviceFeatures m_physicalDeviceFeatures; // availableFeatures

        int32_t m_graphicsQueueIndex;
        int32_t m_presentQueueIndex;
        VkDevice m_device = VK_NULL_HANDLE;

        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue  = VK_NULL_HANDLE;

        VkSurfaceCapabilitiesKHR m_surfaceCapabilities;
        VkSurfaceFormatKHR       m_surfaceFormat;
        VkExtent2D               m_swapchainSize;
        uint32_t                 m_additionalImages = 0;
        VkSwapchainKHR           m_swapchain = VK_NULL_HANDLE;
        VkPresentModeKHR         m_presentMode;

        std::vector<VkImage>     m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;

        VkFormat                 m_depthFormat = VK_FORMAT_UNDEFINED;
        VkImage                  m_depthStencil            = VK_NULL_HANDLE;
        VkImageView              m_depthStencilImageView   = VK_NULL_HANDLE;
        VkDeviceMemory           m_depthStencilImageMemory = VK_NULL_HANDLE;

        VkRenderPass             m_renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_swapchainFrameBuffers;
        std::vector<VkCommandPool> m_commandPools;

        std::vector<VkFence>     m_fences;
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderCompleteSemaphores;

        VkDebugReportCallbackEXT m_debugCallback = VK_NULL_HANDLE;

        std::vector<Frame> m_frames;
    private:
        SDL_Window* _createWindow();
        VkInstance  _createInstance();
        VkPhysicalDevice _selectPhysicalDevice();
        void _selectQueueFamily();
        VkDevice _createDevice();
        void _createSwapchain(uint32_t additionalImages);
        void _destroySwapchain(bool destroyRenderpass);
        void _createDebug();

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
#endif
