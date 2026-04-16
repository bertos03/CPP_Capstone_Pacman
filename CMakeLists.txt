# PROJECT COMMENT (build system)
# -----------------------------------------------------------------------------
# This CMake file describes how BobMan is built and linked against SDL2
# components. The flow is: collect sources -> create target -> attach
# libraries. This keeps the build reproducible and easy for newcomers to follow.

cmake_minimum_required(VERSION 3.11.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-std=c++17)

project(BobMan VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

set(APP_TARGET_NAME BobMan)
set(APP_INSTALL_DATA_DIR "share/bobman")
set(APP_ICON_FILE "${CMAKE_SOURCE_DIR}/cmake/BobMan.icns")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH
        "Local install path used by the IDE install target."
        FORCE)
endif()

# Set default build type to Debug if not specified
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# Set the possible values of build type for cmake-gui
if(NOT CMAKE_CONFIGURATION_TYPES)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

set(PACMAN_BUILD_APP_BUNDLE_DEFAULT OFF)
if(APPLE)
    set(PACMAN_BUILD_APP_BUNDLE_DEFAULT ON)
endif()
option(PACMAN_BUILD_APP_BUNDLE
    "Build the macOS target as a .app bundle instead of a plain executable."
    ${PACMAN_BUILD_APP_BUNDLE_DEFAULT})
option(BOBMAN_ENABLE_CPACK
    "Enable CPack packaging generation during configure."
    OFF)

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
if(APPLE AND PACMAN_BUILD_APP_BUNDLE)
    set_source_files_properties(${APP_ICON_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(${APP_TARGET_NAME} MACOSX_BUNDLE ${project_SRCS} ${APP_ICON_FILE})
    set_target_properties(${APP_TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.bobman"
        MACOSX_BUNDLE_BUNDLE_NAME "BobMan"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "BobMan.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
    )
    add_custom_command(TARGET ${APP_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_BUNDLE_CONTENT_DIR:${APP_TARGET_NAME}>/Resources/data
    )
else()
    add_executable(${APP_TARGET_NAME} ${project_SRCS})
endif()

target_compile_definitions(${APP_TARGET_NAME} PRIVATE PACMAN_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

#string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
target_link_libraries(${APP_TARGET_NAME} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2TTF_LIBRARY} ${SDL2_MIXER_LIBRARIES})

message ([STATUS] "SDL2_INCLUDE_DIR" ${SDL2_INCLUDE_DIR})
message ([STATUS] "SDL2_LIBRARY" ${SDL2_LIBRARY})
message ([STATUS] "SDL2_IMAGE_INCLUDE_DIR" ${SDL2_IMAGE_INCLUDE_DIR})
message ([STATUS] "SDL2_IMAGE_LIBRARY" ${SDL2_IMAGE_LIBRARY})
message ([STATUS] "SDL2TTF_INCLUDE_DIR" ${SDL2TTF_INCLUDE_DIR})
message ([STATUS] "SDL2TTF_LIBRARY" ${SDL2TTF_LIBRARY})
message ([STATUS] "SDL2_MIXER_INCLUDE_DIRS" ${SDL2_MIXER_INCLUDE_DIRS})
message ([STATUS] "SDL2_MIXER_LIBRARIES" ${SDL2_MIXER_LIBRARIES})

if(APPLE AND PACMAN_BUILD_APP_BUNDLE)
    install(TARGETS ${APP_TARGET_NAME} BUNDLE DESTINATION .)

    set(PACMAN_BUNDLE_SEARCH_DIRS "")
    foreach(dep IN LISTS SDL2_LIBRARIES SDL2_IMAGE_LIBRARIES SDL2TTF_LIBRARY SDL2_MIXER_LIBRARIES)
        if(IS_ABSOLUTE "${dep}" AND EXISTS "${dep}")
            get_filename_component(dep_dir "${dep}" DIRECTORY)
            list(APPEND PACMAN_BUNDLE_SEARCH_DIRS "${dep_dir}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES PACMAN_BUNDLE_SEARCH_DIRS)

    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/FixupBundle.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/FixupBundle.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/FixupBundle.cmake")
else()
    install(TARGETS ${APP_TARGET_NAME} RUNTIME DESTINATION bin)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/data" DESTINATION "${APP_INSTALL_DATA_DIR}")
endif()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "BobMan")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "BobMan Game")
set(CPACK_PACKAGE_VENDOR "Your Name")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackConfig.cmake")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackSourceConfig.cmake")

if(APPLE AND PACMAN_BUILD_APP_BUNDLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "BobMan")
    set(CPACK_DMG_FORMAT "UDZO")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP")
endif()

if(BOBMAN_ENABLE_CPACK)
    include(CPack)
endif()
