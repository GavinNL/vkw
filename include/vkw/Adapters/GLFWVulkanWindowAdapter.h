#ifndef VKW_GLFW_VULKAN_WINDOW_ADAPTER_H
#define VKW_GLFW_VULKAN_WINDOW_ADAPTER_H

#include "../vulkan_include.h" // this must be included before glfw3
#include <GLFW/glfw3.h>

#include "VulkanWindowAdapter.h"
#include <vector>
#include <string>

namespace vkw
{

struct GLFWVulkanWindowAdapter : public VulkanWindowAdapater
{
    GLFWwindow * m_window = nullptr;

    struct GLFWUserPointer
    {
        bool requiresResize = false;
        VkExtent2D windowExtent;
    };
    GLFWUserPointer    * m_userPtr = nullptr;

    void createWindow(const char * title, int w, int h )
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_window = glfwCreateWindow(w, h, title, nullptr, nullptr);

        m_userPtr = new GLFWUserPointer();
        m_userPtr->requiresResize = false;
        m_userPtr->windowExtent   = { static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
        glfwSetFramebufferSizeCallback(m_window, GLFWVulkanWindowAdapter::framebufferResizeCallback);
        glfwSetWindowUserPointer(m_window, m_userPtr);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int w, int h)
    {
        auto usrPtr = static_cast<GLFWUserPointer*>(glfwGetWindowUserPointer(window));
        usrPtr->requiresResize = true;
        usrPtr->windowExtent   = { static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    }
    bool requiresResize() const
    {
        return m_userPtr->requiresResize;
    }
    void clearRequireResize()
    {
        m_userPtr->requiresResize = false;
    }

    void destroy()
    {
        if(m_window)
        {
            glfwDestroyWindow(m_window);
        }
        if(m_userPtr)
            delete m_userPtr;
        m_window = nullptr;

    }
    //=================================================================================
    // These functions must be overidden window manager
    //=================================================================================
    VkSurfaceKHR createSurface(VkInstance instance) override
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkResult err = glfwCreateWindowSurface(instance, m_window, NULL, &surface);

        return surface;
    }
    /**
     * @brief getRequiredVulkanExtensions
     * @return
     *
     * Get the required vulkan extensions required to
     * be able to present to SDL windows
     */
    std::vector<std::string> getRequiredVulkanExtensions() override
    {
        std::vector<std::string> outExtensions;

        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for(size_t i=0;i<glfwExtensionCount;i++)
        {
            outExtensions.push_back( glfwExtensions[i] );
        }

        // Add debug display extension, we need this to relay debug messages
        outExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        return outExtensions;
    }

    VkExtent2D getDrawableSize() override
    {
        int  iwidth,iheight = 0;
        glfwGetWindowSize(m_window, &iwidth, &iheight);

        VkExtent2D ext;
        ext.width  = static_cast<uint32_t>(iwidth);
        ext.height = static_cast<uint32_t>(iheight);

        return ext;
    }

};


}

#endif
