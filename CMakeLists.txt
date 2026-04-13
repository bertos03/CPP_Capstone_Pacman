# PROJECT COMMENT (build system)
# -----------------------------------------------------------------------------
# This CMake file describes how BobMan is built and linked against SDL2
# components. The flow is: collect sources -> create target -> attach
# libraries. This keeps the build reproducible and easy for newcomers to follow.

cmake_minimum_required(VERSION 3.11.3)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-std=c++17)

project(pacman)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# Set the possible values of build type for cmake-gui
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)


find_package(SDL2 REQUIRED)
find_package(SDL2TTF REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
#include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS}  src)
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIRS} src)

# Find all executables 
file(GLOB project_SRCS src/*.cpp) #src/*.h

# Add project executable
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # For Release build, create a macOS app bundle
    add_executable(pacman MACOSX_BUNDLE ${project_SRCS})
    set_target_properties(pacman PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.pacman"
        MACOSX_BUNDLE_BUNDLE_NAME "Pacman"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
    )
    # Copy data directory to the app bundle
    add_custom_command(TARGET pacman POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_BUNDLE_CONTENT_DIR:pacman>/Resources/data
    )
else()
    # For Debug build, create regular executable
    add_executable(pacman ${project_SRCS})
endif()

#string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
target_link_libraries(pacman ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2TTF_LIBRARY} ${SDL2_MIXER_LIBRARIES})

message ([STATUS] "SDL2_INCLUDE_DIR" ${SDL2_INCLUDE_DIR})
message ([STATUS] "SDL2_LIBRARY" ${SDL2_LIBRARY})
message ([STATUS] "SDL2_IMAGE_INCLUDE_DIR" ${SDL2_IMAGE_INCLUDE_DIR})
message ([STATUS] "SDL2_IMAGE_LIBRARY" ${SDL2_IMAGE_LIBRARY})
message ([STATUS] "SDL2TTF_INCLUDE_DIR" ${SDL2TTF_INCLUDE_DIR})
message ([STATUS] "SDL2TTF_LIBRARY" ${SDL2TTF_LIBRARY})
message ([STATUS] "SDL2_MIXER_INCLUDE_DIRS" ${SDL2_MIXER_INCLUDE_DIRS})
message ([STATUS] "SDL2_MIXER_LIBRARIES" ${SDL2_MIXER_LIBRARIES})

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "Pacman")
set(CPACK_PACKAGE_VERSION "1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pacman Game")
set(CPACK_PACKAGE_VENDOR "Your Name")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "Pacman")
    set(CPACK_DMG_FORMAT "UDZO")
endif()

include(CPack)
