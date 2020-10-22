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
# this is only needed for the examples
conan install .. -s compiler.libcxx=libstdc++11

# Run cmake and point the cmake prefix path to the location
# where Qt 5.15 is located. This is only needed
# for buliding the examples.
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt5.14

make
```


## Quick Start Code

The following snippet shows how to use the `vkw::SDLVulkanWindow`
in its raw form.

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

// inline implementations for SDL
// only need to include this in one cpp file
#include <vkw/SDLVulkanWindow_INIT.inl>
#include <vkw/SDLVulkanWindow_USAGE.inl>


```


## Using the Application Widget

The application widget is a virtual class which allows you to write your vulkan
rendering code. And then use one of the window manager widgets (SDL/Qt)
provide the main rendering loop.

```c++

#include <vkw/VulkanApplication.h>

class MyApplication : public Application
{
  public:

    void initResources() override
    {
      // called once at the start. Use this to
      // allocate any resources you might need.

    }
    void releaseResources() override
    {
       // called when the window is closed
       // use this to free your resources.
    }

    void initSwapChainResources() override
    {
        // this is called when the swap chain has changed size
        // use this to alllocate any resources that might depend on the  
        // swapchain images.
    }
    void releaseSwapChainResources() override
    {
        // called when the swapchain has changed size. Use this to free
        // any resources you have allocated in initSwapChainResources()
    }

    void render(Frame &frame) override
    {
        // the main rendering loop.
        // frame contains most of the information you would need

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

    // put the window in the main loop
    // and provide a callback function for the SDL events
    vulkanWindow.exec(&app,
                      [&app](SDL_Event const & evt)
    {
        if( evt.type == SDL_QUIT)
            app.quit();
    });

    vulkanWindow.destroy();
    SDL_Quit();
    return 0;
}

// inline implementations for SDL
// only need to include this in one cpp file
#include <vkw/SDLVulkanWindow_INIT.inl>
#include <vkw/SDLVulkanWindow_USAGE.inl>

```


### Using the Qt Wiget

Using the Qt Wiget is a little more involved. You will need Qt 5.15

```c++

#include <vkw/QtVulkanWidget.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    messageLogWidget = new QPlainTextEdit(QLatin1String(QLibraryInfo::build()) + QLatin1Char('\n'));
    messageLogWidget->setReadOnly(true);

    oldMessageHandler = qInstallMessageHandler(messageHandler);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;

    inst.setLayers(QByteArrayList() << VK_LAYER_LUNARG_standard_validation");

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    //=============================================================
    // This the main VulkanWidget
    //=============================================================
    // create a QtVulkanWidget and
    // set the vulkan instance
    vkw::QtVulkanWidget * vulkanWindow = new vkw::QtVulkanWidget();
    vulkanWindow->setVulkanInstance(&inst);

    // create a wrapper for the vulkanWindow
    QWidget * wrapper = QWidget::createWindowContainer(vulkanWindow);

    // Create an instance of our application.
    // this is the actual app that will be
    // performing all the rendering
    MyApplication * myApp = new MyApplication();

    vulkanWindow->init( myApp );

    auto l = new QHBoxLayout();

    auto centralWidget = new QWidget();
    centralWidget->setLayout(l);

    m_window->setCentralWidget(centralWidget);

        l->addWidget(wrapper, 5);
        l->addWidget(new QPushButton(m_window), 1);

    m_window->resize(1024,768);
    m_window->show();

    return app.exec();
}

```
