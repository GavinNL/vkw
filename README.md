# VKW - Vulkan Windows/Widgets

This is a simple quick implementation to set up a Vulkan Window using SDL or GLFW

This was tested on Linux Mint 20,
although I'm sure the code will work on Windows with few changes.
If you discover any issues in windows,
please do send merge requests.

## Features

VKW handles a number of tedious features which are usually required when
creating vulkan applications.
The features provided are listed below:

 * Provides easy ways to enable extensions.
 * Creates the Vulkan window/swapchain
 * Creates a default render pass and optional depth image
 * Handles acquiring of new frame images
 * Handles resizing the window and rebuilding the swapchain images

## Usage

To use this library you can add the repo as a git submodule

```
# add the submodule
add_subdirectories( third_party/vkw)

# Link to the vkw target
target_link_libraries( myapplication vkw::vkw)
```

Follow the example outline in `example.cpp` to get started.

## Build the Example on Linux

```bash
cd vkw
mkdir build && cd build

# run the conan install to get the SDL dependency
# this is only needed for the examples
conan install .. -s compiler.libcxx=libstdc++11

# Run cmake and point the cmake prefix path to the location
# where Qt 5.15 is located. This is only needed
# for buliding the examples.
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt5.15

cmake --build .
```

## Build the Example on Windows

```bash
cd vkw
mkdir build && cd build

# run the conan install to get the SDL dependency
# this is only needed for the examples
conan install ../conanfile_win.txt

# Run cmake and point the cmake prefix path to the location
# where Qt 5.15 is located. This is only needed
# for buliding the examples.
cmake .. -DCMAKE_MODULE_PATH=$PWD -DCMAKE_PREFIX_PATH=/path/to/Qt5.15

cmake --build .
```

# Quick Start

VKW provides two classes to set up Vulkan windows: SDL and GLFW.
Both classes are very similar to set up and use.

The following two sections show the code to set up a vulkan window using
both libraries. As you can see, the code for each of them is very similar.

## Quick Start: SDL

```cpp
#include <iostream>
#include <vkw/VKWVulkanWindow.h>
#include <vkw/Adapters/SDLVulkanWindowAdapter.h>

// Need this in at least one cpp file
#include <vkw/VKWVulkanWindow.inl>

int main()
{
  // This needs to be called first to initialize SDL
  SDL_Init(SDL_INIT_EVERYTHING);

  // create a default window and initialize all vulkan
  // objects.
  auto window = new vkw::VKWVulkanWindow();
  auto sdl_window = new vkw::SDLVulkanWindowAdapter();

  // 1. create the window and set the adapater
  sdl_window->createWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,768);
  window->setWindowAdapater(sdl_window);


  // 2. Create the Instance
  vkw::VKWVulkanWindow::InstanceInitilizationInfo2 instanceInfo;
  instanceInfo.debugCallback = &VulkanReportFunc;
  instanceInfo.vulkanVersion = VK_MAKE_VERSION(1, 0, 0);
  instanceInfo.enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  window->createVulkanInstance(instanceInfo);

  // 3. Create the surface
  vkw::VKWVulkanWindow::SurfaceInitilizationInfo2 surfaceInfo;
  surfaceInfo.depthFormat          = VK_FORMAT_D32_SFLOAT_S8_UINT;
  surfaceInfo.presentMode          = VK_PRESENT_MODE_FIFO_KHR;
  surfaceInfo.additionalImageCount = 1;// how many additional swapchain images should we create ( total = min_images + additionalImageCount
  window->createVulkanSurface(surfaceInfo);


  // 4. Create the device
  //    and add additional extensions that we want to enable
  window->createVulkanPhysicalDevice();

  vkw::VKWVulkanWindow::DeviceInitilizationInfo2 deviceInfo;
  deviceInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  deviceInfo.deviceExtensions.push_back(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

  // enable a new extended feature
  VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexState = {};
  dynamicVertexState.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
  dynamicVertexState.vertexInputDynamicState  = true;
  deviceInfo.enabledFeatures12.pNext          = &dynamicVertexState;

  window->createVulkanDevice(deviceInfo);

  bool running=true;
  while(running)
  {
      SDL_Event event;
      bool resize=false;
      while (SDL_PollEvent(&event))
      {
          if (event.type == SDL_QUIT)
          {
              running = false;
          }
          else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
                  /*&& event.window.windowID == SDL_GetWindowID( window->getSDLWindow()) */ )
          {
              resize=true;
          }
      }
      if( resize )
      {
          // If the window has changed size. we need to rebuild the swapchain
          // and any other textures (depth texture)
          window->rebuildSwapchain();
      }

      // Get the next available frame.
      // the Frame struct is simply a POD containing
      // all the information that you need to record a command buffer
      auto frame = window->acquireNextFrame();


      frame.beginCommandBuffer();
          frame.clearColor = {{1.f,0.f,0.f,0.f}};
          frame.beginRenderPass( frame.commandBuffer );

          // record to frame.commandbuffer

          frame.endRenderPass(frame.commandBuffer);
      frame.endCommandBuffer();

      window->submitFrame(frame);

      // Present the frame after you have recorded
      // the command buffer;
      window->presentFrame(frame);
      window->waitForPresent();
  }



  // delete the window to destroy all objects
  // that were created.
  window->destroy();
  delete window;

  SDL_Quit();
  return 0;
}
```

## Quick Start: GLFW

```cpp
#include <iostream>
#include <vkw/VKWVulkanWindow.h>
#include <vkw/Adapters/GLFWVulkanWindowAdapter.h>

// Need this in at least one cpp file
#include <vkw/VKWVulkanWindow.inl>

int main()
{
  // This needs to be called first to initialize SDL
  glfwInit();

  // create a default window and initialize all vulkan
  // objects.
  auto window      = new vkw::VKWVulkanWindow();
  auto glfw_window = new vkw::GLFWVulkanWindowAdapter();

  // 1. create the window and set the adapater
  glfw_window->createWindow("Title", 1024,768);
  window->setWindowAdapater(glfw_window);


  // 2. Create the Instance
  vkw::VKWVulkanWindow::InstanceInitilizationInfo2 instanceInfo;
  instanceInfo.debugCallback = &VulkanReportFunc;
  instanceInfo.vulkanVersion = VK_MAKE_VERSION(1, 0, 0);
  instanceInfo.enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  window->createVulkanInstance(instanceInfo);

  // 3. Create the surface
  vkw::VKWVulkanWindow::SurfaceInitilizationInfo2 surfaceInfo;
  surfaceInfo.depthFormat          = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
  surfaceInfo.presentMode          = VK_PRESENT_MODE_FIFO_KHR;
  surfaceInfo.additionalImageCount = 1;// how many additional swapchain images should we create ( total = min_images + additionalImageCount
  window->createVulkanSurface(surfaceInfo);

  window->createVulkanPhysicalDevice();

  // 4. Create the device
  //    and add additional extensions that we want to enable
  vkw::VKWVulkanWindow::DeviceInitilizationInfo2 deviceInfo;
  deviceInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  deviceInfo.deviceExtensions.push_back(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

  // enable a new extended feature
  VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexState = {};
  dynamicVertexState.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
  dynamicVertexState.vertexInputDynamicState  = true;
  deviceInfo.enabledFeatures12.pNext          = &dynamicVertexState;

  window->createVulkanDevice(deviceInfo);


  bool running = true;
  while (!glfwWindowShouldClose(glfw_window->m_window) )
  {
      glfwPollEvents();

      bool resize = glfw_window->requiresResize();
      glfw_window->clearRequireResize();

      if( resize )
      {
          // If the window has changed size. we need to rebuild the swapchain
          // and any other textures (depth texture)
          window->rebuildSwapchain();
      }

      // Get the next available frame.
      // the Frame struct is simply a POD containing
      // all the information that you need to record a command buffer
      auto frame = window->acquireNextFrame();


      frame.beginCommandBuffer();
          frame.clearColor = {{1.f,0.f,0.f,0.f}};
          frame.beginRenderPass( frame.commandBuffer );

          // record to frame.commandbuffer

          frame.endRenderPass(frame.commandBuffer);
      frame.endCommandBuffer();

      window->submitFrame(frame);

      // Present the frame after you have recorded
      // the command buffer;
      window->presentFrame(frame);
      window->waitForPresent();
  }

  // delete the window to destroy all objects
  // that were created.
  window->destroy();
  delete window;

  glfwTerminate();
  return 0;
}
```


## Quick Start: Using the Application Widget

The application widget is a virtual class which allows you to write your vulkan
rendering code. And then use one of the window manager widgets (GLFW,SDL/Qt)
provide the main rendering loop.

This was modeled after how Qt handles vulkan rendering. See `example_widget.cpp`
for more info.

To create your vulkan application, you have to inherit from the `vkw::Application`
class and provide overloads for 5 functions:

```c++

#include <vkw/VulkanApplication.h>
#include <iostream>

class MyApplication : public vkw::Application
{
    // Application interface
public:
    // Called once during start up. Use this to
    // initialize any resources you might want to use
    void initResources() override
    {
        // The following can be used here
        // getDevice();
        // getPhysicalDevice();
        // getInstance();
        std::cout << "initResources() " << std::endl;
    }
    // Called once during shutdown. Use this to release any  
    // vulkan resources you have created in initResources()
    void releaseResources() override
    {
        // The following can be used here
        // getDevice();
        // getPhysicalDevice();
        // getInstance();
        std::cout << "releaseResources() " << std::endl;
    }
    // Called when the swapchain changes size. Use this to
    // allocate any resources that may depend on the swapchain images
    // such as descriptor sets, etc.
    void initSwapChainResources() override
    {
        // The following can be used here
        // swapchainImageCount();
        // swapchainImage( index );
        // colorFormat();
        // depthStencilFormat();
        // swapchainImageSize();
        // swapchainImageView();
        std::cout << "initSwapchainResources() " << std::endl;
    }
    // called when the swapchain changes size. This is called
    // before  initSwapChainResources(). Use this to
    // release any resources that depend on the swapchain images .
    void releaseSwapChainResources() override
    {
        std::cout << "releaseSwapChainResources() " << std::endl;
    }
    // The main render function.
    void render( vkw::Frame &frame) override
    {
        assert( frame.depthImage != VK_NULL_HANDLE);

        frame.clearColor.float32[0] = 0.0f;
        //frame.clearColor.float32[1] = 1.0f;
        //frame.clearColor.float32[2] = 1.0f;
        //frame.clearColor.float32[3] = 1.0f;
        frame.beginRenderPass( frame.commandBuffer );

        frame.endRenderPass(frame.commandBuffer);

        // request the next frame
        // so that this function will be called again
        requestNextFrame();
    }
};

```

### Using the Widgets

The widgets are are wrappers around each of the window handling codes.
You can simply choose which window manager you want to use and it will
handle the render loops.

See `example_widget.cpp` to see how GLFW/SDL can be used.
The different window manager is set using a compile-time constant.

See `example_widget_qt.cpp` to see how to use the application in a Qt application.
