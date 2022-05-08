#ifndef VKW_VULKAN_WINDOW_PROFILE_H
#define VKW_VULKAN_WINDOW_PROFILE_H

#include "vulkan_include.h"

#include <vector>
#include <string>
#include "VKWVulkanWindow.h"

namespace vkw
{

class VKWVulkanWindowProfile : public VKWVulkanWindow
{
public:

    void createProfileInstance(VpProfileProperties const & props,
                               std::vector<std::string> const & additionalExtensions,
                               std::vector<std::string> const & layers)
    {
        VkBool32 profile_supported;
        vpGetInstanceProfileSupport(nullptr, &props, &profile_supported);
        if (!profile_supported)
        {
            exit(1);
        }

        std::vector<char const*> enabled_layers;
        std::vector<char const*> enabled_extensions;
        for(auto & x : additionalExtensions)
            enabled_extensions.push_back(x.data());
        for(auto & x : layers)
            enabled_layers.push_back(x.data());

        VkInstanceCreateInfo create_info{};
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.ppEnabledExtensionNames = enabled_extensions.data();
        create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size());
        create_info.enabledLayerCount       = static_cast<uint32_t>(enabled_layers.size());
        create_info.ppEnabledLayerNames     = enabled_layers.data();

        VpInstanceCreateInfo instance_create_info{};
        instance_create_info.pProfile    = &props;
        instance_create_info.pCreateInfo = &create_info;
        instance_create_info.flags       = VP_INSTANCE_CREATE_MERGE_EXTENSIONS_BIT;

        VkInstance vulkan_instance = VK_NULL_HANDLE;
        auto result = vpCreateInstance(&instance_create_info, nullptr, &vulkan_instance);
        assert(result == VK_SUCCESS);
        setInstance(vulkan_instance);
    }

    void createProfileDevice(VkPhysicalDevice pd,
                             VkSurfaceKHR surface,
                             VpProfileProperties const & profile_properties,
                             std::vector<std::string> const extraRequiredDeviceExtenstions = {})
    {
        m_physicalDevice = pd;
        m_surface = surface;

        // Check if the profile is supported at device level
        VkBool32 profile_supported;
        vpGetPhysicalDeviceProfileSupport(getInstance(), pd, &profile_properties, &profile_supported);
        if (!profile_supported)
        {
            throw std::runtime_error{"The selected profile is not supported (error at creating the device)!"};
        }


        _selectQueueFamily();

        const float queue_priority[] = { 1.0f };
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

        std::vector<const char*> deviceExtensions;
        for(auto & x : extraRequiredDeviceExtenstions)
        {
            deviceExtensions.push_back(x.data());
        }

        VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        create_info.pNext                   = nullptr;
        create_info.pQueueCreateInfos       = queueCreateInfos.data();
        create_info.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
        create_info.pEnabledFeatures        = nullptr;
        create_info.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
        create_info.ppEnabledExtensionNames = deviceExtensions.data();

        // Create the device using the profile tool library
        VpDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.pCreateInfo = &create_info;
        deviceCreateInfo.pProfile    = &profile_properties;
        deviceCreateInfo.flags       = VP_DEVICE_CREATE_MERGE_EXTENSIONS_BIT;
        VkDevice vulkan_device;
        VkResult result = vpCreateDevice(pd, &deviceCreateInfo, nullptr, &vulkan_device);

        assert(result == VK_SUCCESS);

        m_device = vulkan_device;

        vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_graphicsQueueIndex), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_presentQueueIndex ), 0, &m_presentQueue);
    }

    void setDebugCallback(PFN_vkDebugReportCallbackEXT callbackfunc)
    {
        if( m_debugCallback )
        {
            auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT"));
            if (func != nullptr)
            {
                func(m_instance, m_debugCallback, nullptr);
            }
            m_debugCallback = nullptr;
        }

        if(!m_debugCallback)
        {
            m_debugCallback = _createDebug(callbackfunc);
        }
    }
};

}

#endif
