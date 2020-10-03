# SDLVulkanWindow

This is a simple quick implementation to set up a Vulkan Window using SDL.

The feature list is not exhaustive and there are few options. 

This was tested on Linux Mint 20, although I'm sure the code will work on Windows with few changes. If you discover any issues in windows, please do send merge requests.

## Usage

Copy the `SDLVulkanWindow.h` `SDLVulkanWindow_INIT.cpp` and `SDLVulkanWindow_USAGE.cpp` into your project folder.

Follow the example outline in `example.cpp` to get started.

## Build the Example

```bash
cd SDLVulkanWindow
mkdir build && cd build

# run the conan install to get the SDL dependency
conan install .. -s compiler.libcxx=libstdc++11

cmake ..

make
```


## Quick Start Code


```C++
int main()
{
    // This needs to be called first to initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // create a default window and initialize 
    auto window = new SDLVulkanWindow();

    // 1. create the window
    window->createWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,768);

    // 2. initialize the vulkan instance using the default options
    SDLVulkanWindow::InitilizationInfo info;
    window->createVulkanInstance( info);

    // 3. Create the surface using default values
    SDLVulkanWindow::SurfaceInitilizationInfo surfaceInfo;
    window->initSurface(surfaceInfo);

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
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED )
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
    delete window;

    SDL_Quit();
    return 0;
}

```
