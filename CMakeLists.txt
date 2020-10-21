cmake_minimum_required(VERSION 3.10)


project( SDLVulkanWindow LANGUAGES CXX)


################################################################################
# If the conan file exists, include it
#
#  Use conan package manager to handle SDL library
################################################################################
if( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake )
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS TARGETS)
endif()
################################################################################


# Needed for vulkan
find_package(Vulkan)


################################################################################
add_library(vkw INTERFACE)
add_library(vkw::vkw ALIAS vkw)
target_include_directories(vkw INTERFACE include)
target_link_libraries( vkw INTERFACE Vulkan::Vulkan)
################################################################################



add_executable( example_SDLWindow example_SDLWindow.cpp )
target_link_libraries(example_SDLWindow  CONAN_PKG::sdl2  vkw::vkw)

add_executable( example_widget example_widget.cpp )
target_link_libraries(example_widget  CONAN_PKG::sdl2  vkw::vkw)

################################################################################

