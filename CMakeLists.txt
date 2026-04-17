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
set(BOBMAN_BUNDLE_IDENTIFIER "com.example.bobman" CACHE STRING
    "macOS bundle identifier used for the BobMan app bundle.")
set(BOBMAN_CODESIGN_IDENTITY "" CACHE STRING
    "Developer ID Application identity used for macOS distribution signing.")
set(BOBMAN_NOTARY_KEYCHAIN_PROFILE "" CACHE STRING
    "Keychain profile name used by xcrun notarytool for macOS notarization.")

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
option(BOBMAN_CREATE_DISTRIBUTABLE_ARCHIVE
    "Create a sanitized macOS ZIP archive for distribution after app bundle builds."
    ${APPLE})
option(BOBMAN_ENABLE_NOTARIZATION
    "Submit the macOS release archive to Apple notarization after signing."
    OFF)

if(APPLE)
    find_program(PACMAN_XATTR_EXECUTABLE xattr)
    find_program(PACMAN_CODESIGN_EXECUTABLE codesign)
    find_program(PACMAN_CP_EXECUTABLE cp)
    find_program(PACMAN_CHMOD_EXECUTABLE chmod)
    find_program(PACMAN_DITTO_EXECUTABLE ditto)
    find_program(PACMAN_XCRUN_EXECUTABLE xcrun)
endif()

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
        MACOSX_BUNDLE_GUI_IDENTIFIER "${BOBMAN_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_NAME "BobMan"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "BobMan.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
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
    set(BOBMAN_LOCAL_RUN_BUNDLE_DIR "/tmp/BobMan-app-${CMAKE_BUILD_TYPE}")
    set(BOBMAN_LOCAL_RUN_BUNDLE_PATH "${BOBMAN_LOCAL_RUN_BUNDLE_DIR}/BobMan.app")

    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND BOBMAN_CREATE_DISTRIBUTABLE_ARCHIVE
       AND NOT BOBMAN_CODESIGN_IDENTITY)
        message(WARNING
            "macOS release builds are currently only ad-hoc signed. "
            "They will run locally, but Gatekeeper on another Mac will still report "
            "that Apple could not check the app for malicious software. "
            "Set BOBMAN_CODESIGN_IDENTITY to a Developer ID Application certificate "
            "and optionally enable BOBMAN_ENABLE_NOTARIZATION.")
    endif()
    if(BOBMAN_ENABLE_NOTARIZATION)
        if(NOT BOBMAN_CODESIGN_IDENTITY)
            message(FATAL_ERROR
                "BOBMAN_ENABLE_NOTARIZATION requires BOBMAN_CODESIGN_IDENTITY "
                "to be set to a Developer ID Application identity.")
        endif()
        if(NOT BOBMAN_NOTARY_KEYCHAIN_PROFILE)
            message(FATAL_ERROR
                "BOBMAN_ENABLE_NOTARIZATION requires BOBMAN_NOTARY_KEYCHAIN_PROFILE.")
        endif()
        if(NOT PACMAN_XCRUN_EXECUTABLE)
            message(FATAL_ERROR
                "BOBMAN_ENABLE_NOTARIZATION requires xcrun / notarytool.")
        endif()
    endif()

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
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/CreateMacOSArchive.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CreateMacOSArchive.cmake"
        @ONLY
    )
    set_target_properties(${APP_TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${BOBMAN_LOCAL_RUN_BUNDLE_DIR}"
    )

    set(BOBMAN_RUN_COMMAND_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/Run BobMan.command")
    file(WRITE "${BOBMAN_RUN_COMMAND_SCRIPT}" "#!/bin/zsh\nset -e\nAPP=\"${BOBMAN_LOCAL_RUN_BUNDLE_PATH}\"\nif [ ! -d \"$APP\" ]; then\n  echo \"Runnable app bundle not found: $APP\" >&2\n  echo \"Build the app first with the matching CMake preset.\" >&2\n  exit 1\nfi\nopen \"$APP\"\n")
    if(PACMAN_CHMOD_EXECUTABLE)
        execute_process(COMMAND "${PACMAN_CHMOD_EXECUTABLE}" +x "${BOBMAN_RUN_COMMAND_SCRIPT}")
    endif()

    add_custom_command(TARGET ${APP_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_TARGET_NAME}>/Resources/data
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_TARGET_NAME}>/Resources/data
        COMMAND ${CMAKE_COMMAND} -E env COPYFILE_DISABLE=1
            ${PACMAN_CP_EXECUTABLE} -RX
            ${CMAKE_SOURCE_DIR}/data/.
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_TARGET_NAME}>/Resources/data/
        COMMAND ${CMAKE_COMMAND}
            -DBOBMAN_BUNDLE_PATH=$<TARGET_BUNDLE_DIR:${APP_TARGET_NAME}>
            -P "${CMAKE_CURRENT_BINARY_DIR}/FixupBundle.cmake"
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            ${CMAKE_CURRENT_BINARY_DIR}/BobMan.app
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${BOBMAN_LOCAL_RUN_BUNDLE_PATH}
            ${CMAKE_CURRENT_BINARY_DIR}/BobMan.app
        VERBATIM
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND BOBMAN_CREATE_DISTRIBUTABLE_ARCHIVE)
        add_custom_command(TARGET ${APP_TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DBOBMAN_SOURCE_BUNDLE_PATH=$<TARGET_BUNDLE_DIR:${APP_TARGET_NAME}>
                -DBOBMAN_OUTPUT_ZIP_PATH=${CMAKE_CURRENT_BINARY_DIR}/dist/BobMan-macOS.zip
                -P "${CMAKE_CURRENT_BINARY_DIR}/CreateMacOSArchive.cmake"
            VERBATIM
        )
    endif()

    install(TARGETS ${APP_TARGET_NAME} BUNDLE DESTINATION .)
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
