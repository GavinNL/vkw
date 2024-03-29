#ifndef VKW_VULKAN_WINDOW_H
#define VKW_VULKAN_WINDOW_H

#include "vulkan_include.h"

#include <vector>
#include <string>
#include "Frame.h"
#include "base_widget.h"
#include "Adapters/VulkanWindowAdapter.h"

namespace vkw
{

class VKWVulkanWindow : public BaseWidget
{
    public:

    struct InstanceInitilizationInfo2
    {
        PFN_vkDebugReportCallbackEXT debugCallback = nullptr;
        std::vector<std::string> enabledLayers     = { "VK_LAYER_KHRONOS_validation"};
        std::vector<std::string> enabledExtensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };

        #if defined VK_HEADER_VERSION_COMPLETE
            #define VKW_DEFAULT_VULKAN_VERSION VK_HEADER_VERSION_COMPLETE
        #else
            #define VKW_DEFAULT_VULKAN_VERSION VK_MAKE_VERSION(1, 0, 0)
        #endif
        uint32_t    vulkanVersion   = VKW_DEFAULT_VULKAN_VERSION;
        std::string applicationName = "App name";
        std::string engineName      = "Engine Name";
    };

    struct SurfaceInitilizationInfo2
    {
        VkFormat         surfaceFormat        = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
        VkFormat         depthFormat          = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
        VkPresentModeKHR presentMode          = VK_PRESENT_MODE_FIFO_KHR;
        uint32_t         additionalImageCount = 1;// how many additional swapchain images should we create ( total = min_images + additionalImageCount
    };

    struct DeviceInitilizationInfo2
    {
        // which device ID do you want to use?
        // set to 0 to choose the first discrete GPU it can find.
        // if no discrete gpu is found, uses the first GPU found
        //
        // auto devices = window->getAvailablePhysicalDevices();
        //   devices[0].deviceID
        uint32_t deviceID = 0;

        std::vector<std::string>         deviceExtensions  = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};


        // the values enabledFeaturesXX structs can be set to true
        // to enable the features. you do not need to set the sNext
        // pointers. They will automatically be set when we create the device

        // if you want enable non KHR extensions, then you will have to set
        // the enabledFeatures12.pNext value yourself
        VkPhysicalDeviceFeatures2        enabledFeatures   = {};
        VkPhysicalDeviceVulkan11Features enabledFeatures11 = {};
        VkPhysicalDeviceVulkan12Features enabledFeatures12 = {};
        VkPhysicalDeviceVulkan13Features enabledFeatures13 = {};

    };

    //=================================================================
    // 1. Create the  window first using this function
    void setWindowAdapater(VulkanWindowAdapater * window);

    // 2. Create a vulkan instance
    void createVulkanInstance(InstanceInitilizationInfo2 const & I);
    void setVulkanInstance(VkInstance instance);

    // 3. create the vulkan surface
    bool createVulkanSurface(SurfaceInitilizationInfo2 const & I);

    // 5. Create the logical device
    void createVulkanDevice(DeviceInitilizationInfo2 const & I);


    void setDepthFormat(VkFormat format)
    {
        m_initInfo2.surface.depthFormat = format;
    }
    void setPresentMode(VkPresentModeKHR mode)
    {
        m_initInfo2.surface.presentMode = mode;
    }
    std::vector<VkPhysicalDeviceProperties> getAvailablePhysicalDevices() const
    {
        return getAvailablePhysicalDevices(m_instance);
    }
    static std::vector<VkPhysicalDeviceProperties> getAvailablePhysicalDevices(VkInstance instance)
    {
        std::vector<VkPhysicalDevice> physicalDevices;
        uint32_t physicalDeviceCount = 0;

        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        physicalDevices.resize(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        std::vector<VkPhysicalDeviceProperties> _p;
        for(auto & pD : physicalDevices)
        {
            VkPhysicalDeviceProperties props;

            vkGetPhysicalDeviceProperties(pD, &props);
            _p.push_back(props);
        }
        return _p;
    }
    static VkPhysicalDeviceProperties getPhysicalDeviceProperties(VkInstance instance, VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        return props;
    }
    static std::vector<VkPhysicalDevice> getPhysicalDevices(VkInstance instance)
    {
        // Querying valid physical devices on the machine
        uint32_t physical_device_count{0};
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

        std::vector<VkPhysicalDevice> physical_devices;
        physical_devices.resize(physical_device_count);
        vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());

        return physical_devices;
    }
    static std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice physicalDevice)
    {
        uint32_t queue_family_properties_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_properties_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_properties_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_properties_count, queue_family_properties.data());
        return queue_family_properties;
    }


    /**
     * @brief getPhysicalDevice
     * @param instance
     * @param surface
     * @return
     *
     * Return a physical device which is suitable to present to surface
     */
    static VkPhysicalDevice getPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
    {
        auto physicalDevices = getPhysicalDevices(instance);

        for(auto pd : physicalDevices)
        {
            //assert(!gpus.empty() && "No physical devices were found on the system.");
            auto props = getPhysicalDeviceProperties(instance, pd);

            // Find a discrete GPU
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                //See if it work with the surface
                auto queue_family_properties = getQueueFamilyProperties(pd);
                size_t queue_count = queue_family_properties.size();
                for (uint32_t queue_idx = 0; static_cast<size_t>(queue_idx) < queue_count; queue_idx++)
                {
                    VkBool32 present_supported = VK_FALSE;
                    vkGetPhysicalDeviceSurfaceSupportKHR(pd, queue_idx, surface, &present_supported);
                    if(present_supported)
                        return pd;
                }
            }
        }
        return VK_NULL_HANDLE;
    }

    //=================================================================


    //=================================================================================
    /**
     * @brief destroy
     *
     * When you are done with the window, you must call this
     * too release any vulkan resources associated with the
     * window.
     */
    void destroy();

    std::vector<std::string> getAvailableVulkanLayers();


    ~VKWVulkanWindow();


    void setInstance(VkInstance instance);

    void setPhysicalDevice(VkPhysicalDevice physicalDevice);

    //===================================================================
    // The implementation for the following functions are in
    //  SDLVulkanWindow_USAGE.cpp
    //===================================================================

    /**
     * @brief acquireNextFrame
     * @return
     *
     * Get the next available frame. The Frame contains
     * information about which swapchain image index to use
     * a handle to a command buffer and the command pool should
     * you need it.
     *
     * It also contains semaphores to handle synhronization
     */
    Frame acquireNextFrame();

    /**
     * @brief submitFrame
     * @param C
     *
     * Submt the the frame to the GPU to process.
     * The command buffer C.commandBuffer will be
     * sent to the GPU for processing.
     */
    void  submitFrame(Frame const & C);

    void  submitFrameCommandBuffer(VkCommandBuffer cb,
                                   VkSemaphore wait,
                                   VkSemaphore signal,
                                   VkFence fence);

    /**
     * @brief presentFrame
     * @param F
     *
     * Present the swapchain frame which was
     * acquired by acquireNextFrame();
     */
    void  presentFrame(const Frame &F);

    /**
     * @brief waitForPresent
     *
     * Wait until the device is idle
     */
    void  waitForPresent();

    //SDL_Window* getSDLWindow() const
    //{
    //    return m_window;
    //}
    VulkanWindowAdapater* getWindowAdapter() const
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
    VkRenderPass getRenderPass() const
    {
        return m_renderPass;
    }
    //===================================================================

    void rebuildSwapchain()
    {
        _destroySwapchain(false);
        _createSwapchain(m_initInfo2.surface.additionalImageCount);
    }

    static VkPhysicalDeviceFeatures2 getSupportedDeviceFeatures(VkPhysicalDevice physicalDevice);
    static VkPhysicalDeviceVulkan11Features getSupportedDeviceFeatures11(VkPhysicalDevice physicalDevice);
    static VkPhysicalDeviceVulkan12Features getSupportedDeviceFeatures12(VkPhysicalDevice physicalDevice);
    static VkPhysicalDeviceVulkan13Features getSupportedDeviceFeatures13(VkPhysicalDevice physicalDevice);
protected:

    template<typename callable_t>
    VkPhysicalDevice chooseVulkanPhysicalDevice(callable_t && callable) const
    {
        using namespace std;
        vector<VkPhysicalDevice> physicalDevices;
        uint32_t physicalDeviceCount = 0;

        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
        physicalDevices.resize(physicalDeviceCount);
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

        for(auto & pD : physicalDevices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(pD, &props);
            if( callable(props))
            {
                return pD;
                break;
            }
        }
        return VK_NULL_HANDLE;
    }

    struct
    {
        InstanceInitilizationInfo2 instance;
        SurfaceInitilizationInfo2  surface;
        DeviceInitilizationInfo2   device;
    } m_initInfo2;

    //SDL_Window *               m_window   = nullptr;
    VulkanWindowAdapater      *m_window   = nullptr;
    VkInstance                 m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR               m_surface  = VK_NULL_HANDLE;
    VkPhysicalDevice           m_physicalDevice;
    int32_t                    m_graphicsQueueIndex;
    int32_t                    m_presentQueueIndex;
    VkDevice                   m_device        = VK_NULL_HANDLE;
    VkQueue                    m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue                    m_presentQueue  = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR   m_surfaceCapabilities;
    VkSurfaceFormatKHR         m_surfaceFormat;
    VkExtent2D                 m_swapchainSize;
    VkSwapchainKHR             m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>       m_swapchainImages;
    std::vector<VkImageView>   m_swapchainImageViews;
    VkImage                    m_depthStencil            = VK_NULL_HANDLE;
    VkImageView                m_depthStencilImageView   = VK_NULL_HANDLE;
    VkDeviceMemory             m_depthStencilImageMemory = VK_NULL_HANDLE;
    VkRenderPass               m_renderPass              = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_swapchainFrameBuffers;
    std::vector<VkCommandPool> m_commandPools;
    std::vector<VkFence>       m_fences;
    std::vector<VkSemaphore>   m_imageAvailableSemaphores;
    std::vector<VkSemaphore>   m_renderCompleteSemaphores;
    VkDebugReportCallbackEXT   m_debugCallback = VK_NULL_HANDLE;
    std::vector<Frame>         m_frames;

protected:
    void             _selectQueueFamily();
    VkDevice         _createDevice();
    void             _createSwapchain(uint32_t additionalImages);
    void             _destroySwapchain(bool destroyRenderpass);
    VkDebugReportCallbackEXT _createDebug(PFN_vkDebugReportCallbackEXT _callback);

    std::pair<VkImage, VkDeviceMemory> createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

    void _createDepthStencil();
    void _createRenderPass();
    void _createFramebuffers();

    void _createPerFrameObjects();
};
}

#endif
