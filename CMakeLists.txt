cmake_minimum_required(VERSION 3.10)


project( vkw LANGUAGES CXX)

# Needed for vulkan
find_package(Vulkan)


################################################################################
# Create an interface for vkw so that it can be
# included as a subdir
################################################################################
add_library(vkw INTERFACE)
add_library(vkw::vkw ALIAS vkw)
target_include_directories(vkw INTERFACE include)
target_link_libraries( vkw INTERFACE Vulkan::Vulkan)
################################################################################

get_directory_property(vkw_has_parent PARENT_DIRECTORY)

option(VKW_BUILD_EXAMPLES "Build examples" ON)

if(vkw_has_parent)

    message("Not building examples")
    set(VKW_BUILD_EXAMPLES OFF)

endif()



if( VKW_BUILD_EXAMPLES )

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

    add_executable( example_SDLWindow example_SDLWindow.cpp )
    target_link_libraries(example_SDLWindow  CONAN_PKG::sdl2  vkw::vkw)

    add_executable( example_widget_sdl example_widget_sdl.cpp )
    target_link_libraries(example_widget_sdl  CONAN_PKG::sdl2  vkw::vkw)


    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt5 COMPONENTS Widgets)

    if (Qt5Widgets_FOUND)

        if (Qt5Widgets_VERSION VERSION_LESS 5.15.0)

            message(NOTICE "Minimum supported Qt5 version is 5.15. Cannot build Qt examples!")

        else()

            add_executable( example_widget_qt example_widget_qt.cpp )
            target_link_libraries(example_widget_qt  Qt5::Widgets pthread  vkw::vkw)

        endif()
    else()
        message(NOTICE "The Qt5Widgets library could not be found! Not building Qt examples")
    endif(Qt5Widgets_FOUND)

endif()

################################################################################
