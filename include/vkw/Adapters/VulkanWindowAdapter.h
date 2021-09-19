#ifndef VKW_VULKAN_WINDOW_ADAPTER_H
#define VKW_VULKAN_WINDOW_ADAPTER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace vkw
{

struct VulkanWindowAdapater
{
    virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;

    virtual std::vector<std::string> getRequiredVulkanExtensions() = 0;

    virtual VkExtent2D getDrawableSize() = 0;
};

}

#endif
