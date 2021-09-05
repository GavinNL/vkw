#ifndef VKW_BASE_WIDGET_H
#define VKW_BASE_WIDGET_H

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkw
{

class BaseWidget
{
public:
    /**
     * @brief getSupportedExtensions
     * @return
     *
     * Return a set of all the extensions currently supported
     * but your vulkan library.
     */
    static std::set<std::string> getSupportedExtensions()
    {
        uint32_t count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
        std::set<std::string> results;
        for (auto & extension : extensions) {
            results.insert(extension.extensionName);
        }
        return results;
    }

    static std::set<std::string> getSupportedLayers()
    {
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr); //get number of extensions
        std::vector<VkLayerProperties> extensions(count);
        vkEnumerateInstanceLayerProperties(&count, extensions.data()); //populate buffer
        std::set<std::string> results;
        for (auto & extension : extensions) {
            results.insert(extension.layerName);
        }
        return results;
    }

    static std::set<std::string> getSupportedDeviceExtensions(VkPhysicalDevice physicalDevice)
    {
        uint32_t count;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data()); //populate buffer
        std::set<std::string> results;
        for (auto & extension : extensions) {
            results.insert(extension.extensionName);
        }
        return results;
    };
    static std::set<std::string> getSupportedDeviceExtensions(VkPhysicalDevice physicalDevice, char const* layer)
    {
        uint32_t count;
        vkEnumerateDeviceExtensionProperties(physicalDevice, layer, &count, nullptr);
        //vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateDeviceExtensionProperties(physicalDevice, layer, &count, extensions.data()); //populate buffer
        std::set<std::string> results;
        for (auto & extension : extensions) {
            results.insert(extension.extensionName);
        }
        return results;
    };

};

}

#endif
