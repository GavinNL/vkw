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


