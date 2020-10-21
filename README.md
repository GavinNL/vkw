# VKW - Vulkan Windows/Widgets

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
#include <vkw/SDLVulkanWindow.h>

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

// inline implementations
#include <vkw/SDLVulkanWindow_INIT.inl>
#include <vkw/SDLVulkanWindow_USAGE.inl>


```


## Using the Application Widget

The application widget is a virtual class which allows you to write your vulkan rendering code. And then use one of the window manager widgets (SDL/Qt)

```c++

#include <vkw/VulkanApplication.h>

class MyApplication : public Application
{
    // Application interface
public:
    void initResources() override
    {
        // initialization function
    }
    void releaseResources() override
    {
        // clean up function
    }
    void initSwapChainResources() override
    {
        
    }
    void releaseSwapChainResources() override
    {
        
    }
    void render(Frame &frame) override
    {
        frame.beginRenderPass( frame.commandBuffer );

        frame.endRenderPass(frame.commandBuffer);

        // request the next frame
        // so that this function will be called again
        requestNextFrame();
    }
};

```

### Using the SDL Widget

You can now run your application using the SDLWidget in the following way.  

```c++
#include <vkw/SDLWidget.h>

int main()
{
    // This needs to be called first to initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // create a vulkan window widget
    SDLVulkanWidget3 vulkanWindow;

    SDLVulkanWidget3::CreateInfo c;
    c.width       = 1024;
    c.height      = 768;
    c.windowTitle = "My Vulkan Application Window";
    c.depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
    c.callback    = &VulkanReportFunc;

    // create the window and initialize
    vulkanWindow.create(c);

    MyApplication app;
    vulkanWindow.exec(&app);

    vulkanWindow.destroy();
    SDL_Quit();
    return 0;
}
```


### Using the Qt Wiget

Using the Qt Wiget is a little more involved. You will need Qt 5.15

```c++

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    messageLogWidget = new QPlainTextEdit(QLatin1String(QLibraryInfo::build()) + QLatin1Char('\n'));
    messageLogWidget->setReadOnly(true);

    oldMessageHandler = qInstallMessageHandler(messageHandler);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;

    inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    //=============================================================
    // This the main VulkanWidget
    //=============================================================
    // This is the widget that can be added to a QtWindow or any other
    // widget. The actual  Applicatioof the application
    vka::QtVulkanWidget2 * vulkanWindow = new vka::QtVulkanWidget2();

    // create an instance of the application
    MyApp * myApp = new MyApp();


    vulkanWindow->setVulkanInstance(&inst);
    vulkanWindow->init( myApp );

    // or we can add it to another widget;
    MainWindow mainWindow(vulkanWindow, messageLogWidget.data());

    mainWindow.resize(1024, 768);
    mainWindow.show();

    return app.exec();
}

```


