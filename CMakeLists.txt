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


file(GLOB_RECURSE srcFiles ${CMAKE_CURRENT_SOURCE_DIR}/* )

add_executable( example example.cpp SDLVulkanWindow_INIT.cpp SDLVulkanWindow_USAGE.cpp )

target_link_libraries(example 
                        PUBLIC 
                            CONAN_PKG::sdl2 
                            Vulkan::Vulkan)

################################################################################

