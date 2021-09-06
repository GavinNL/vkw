cmake_minimum_required(VERSION 3.10)

project( vkw LANGUAGES CXX)

################################################################################
# Create an interface for vkw so that it can be
# included as a subdir
################################################################################
add_library(vkw INTERFACE)
add_library(vkw::vkw ALIAS vkw)
target_include_directories(vkw INTERFACE include)
target_link_libraries( vkw INTERFACE)
################################################################################

get_directory_property(vkw_has_parent PARENT_DIRECTORY)

option(VKW_BUILD_EXAMPLES "Build examples" ON)

if(vkw_has_parent)
    # Don't build examples if we are a submodule.
    message("Not building examples")
    set(VKW_BUILD_EXAMPLES OFF)

endif()



if( VKW_BUILD_EXAMPLES )

    # Set the current binary directory
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_MODULE_PATH})

    # Needed for vulkan
    find_package(Vulkan REQUIRED)
    find_package(SDL2 REQUIRED)

    add_executable( example_SDLWindow example_SDLWindow.cpp )
    target_link_libraries(example_SDLWindow  SDL2::SDL2  vkw::vkw Vulkan::Vulkan)

    add_executable( example_SDLWindow_2 example_SDLWindow_2.cpp )
    target_link_libraries(example_SDLWindow_2  SDL2::SDL2  vkw::vkw Vulkan::Vulkan)

    add_executable( example_widget_sdl example_widget_sdl.cpp )
    target_link_libraries(example_widget_sdl  SDL2::SDL2  vkw::vkw Vulkan::Vulkan)


    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # First look for Qt6
    find_package(Qt6 COMPONENTS Widgets)

    if (QT6_INSTALL_PREFIX)

        add_executable( example_widget_qt example_widget_qt.cpp )
        target_link_libraries(example_widget_qt  Qt6::Widgets pthread  vkw::vkw vkw::vkw Vulkan::Vulkan)

    else()

        find_package(Qt5 COMPONENTS Widgets)

        if( Qt5Widgets_FOUND )

            if (Qt5Widgets_VERSION VERSION_LESS 5.15.0)
                message(NOTICE "Minimum supported Qt5 version is 5.15. Cannot build Qt examples!")
            else()
                add_executable( example_widget_qt example_widget_qt.cpp )
                target_link_libraries(example_widget_qt  Qt5::Widgets pthread  vkw::vkw vkw::vkw Vulkan::Vulkan)
            endif()

        else()
            message(NOTICE "The Qt5 library could not be found! Not building Qt examples")
        endif()

    endif()

endif()

################################################################################
