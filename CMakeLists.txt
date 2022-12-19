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


################################################################################
# Only create examples if this project is a root project
# if it is included as a subdirectory it will not create the examples
################################################################################
get_directory_property(vkw_has_parent PARENT_DIRECTORY)
if(vkw_has_parent)
    # Don't build examples if we are a submodule.
    message("Not building examples")
    option(VKW_BUILD_EXAMPLES "Build examples" OFF)
else()
    option(VKW_BUILD_EXAMPLES "Build examples" ON)
endif()
################################################################################



if( VKW_BUILD_EXAMPLES )

    # Set the current binary directory
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_MODULE_PATH})

    # Needed for vulkan
    find_package(Vulkan REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(glfw3 REQUIRED)

    SET(vulkan_libs  Vulkan::Vulkan)

    add_executable( example_GLFWWindow examples/example_GLFWWindow.cpp )
    target_link_libraries(example_GLFWWindow  glfw::glfw  vkw::vkw ${vulkan_libs})

    add_executable( example_SDLWindow examples/example_SDLWindow.cpp )
    target_link_libraries(example_SDLWindow  SDL2::SDL2  vkw::vkw ${vulkan_libs} )

    add_executable( example_SDLWindow_vulkanProfiles examples/example_SDLWindow_vulkanProfiles.cpp )
    target_link_libraries(example_SDLWindow_vulkanProfiles SDL2::SDL2  vkw::vkw ${vulkan_libs} )

    add_executable( example_widget_sdl examples/example_widget.cpp )
    target_link_libraries(example_widget_sdl  SDL2::SDL2  vkw::vkw ${vulkan_libs})
    target_compile_definitions(example_widget_sdl PRIVATE VKW_WINDOW_LIB=1)

    add_executable( example_widget_glfw examples/example_widget.cpp )
    target_link_libraries(example_widget_glfw  glfw::glfw  vkw::vkw ${vulkan_libs})
    target_compile_definitions(example_widget_glfw PRIVATE VKW_WINDOW_LIB=2)


    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # First look for Qt6
    find_package(Qt6 COMPONENTS Widgets)

    if (QT6_INSTALL_PREFIX)

        add_executable( example_widget_qt examples/example_widget_qt.cpp )
        target_link_libraries(example_widget_qt  Qt6::Widgets pthread  vkw::vkw ${vulkan_libs})

    else()

        find_package(Qt5 COMPONENTS Widgets)

        if( Qt5Widgets_FOUND )

            if (Qt5Widgets_VERSION VERSION_LESS 5.15.0)
                message(NOTICE "Minimum supported Qt5 version is 5.15. Cannot build Qt examples!")
            else()
                add_executable( example_widget_qt examples/example_widget_qt.cpp )
                target_link_libraries(example_widget_qt  Qt5::Widgets pthread  vkw::vkw ${vulkan_libs})
            endif()

        else()
            message(NOTICE "The Qt5 library could not be found! Not building Qt examples")
        endif()

    endif()

endif()

################################################################################
