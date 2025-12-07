# SDL3_net CMake configuration file:
# This file is meant to be placed in Resources/CMake of a SDL3_net framework

# INTERFACE_LINK_OPTIONS needs CMake 3.12
cmake_minimum_required(VERSION 3.12...4.0)

include(FeatureSummary)
set_package_properties(SDL3_net PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_net/"
    DESCRIPTION "SDL_net is a portable networking library"
)

set(SDL3_net_FOUND TRUE)

# Compute the installation prefix relative to this file:
# search upwards for the .framework directory
set(_current_path "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(_current_path "${_current_path}" REALPATH)
set(_sdl3_net_framework_path "")

while(NOT _sdl3_net_framework_path)
    if(IS_DIRECTORY "${_current_path}" AND "${_current_path}" MATCHES "/SDL3_net\\.framework$")
        set(_sdl3_net_framework_path "${_current_path}")
        break()
    endif()
    get_filename_component(_next_current_path "${_current_path}" DIRECTORY)
    if("${_current_path}" STREQUAL "${_next_current_path}")
        break()
    endif()
    set(_current_path "${_next_current_path}")
endwhile()
unset(_current_path)
unset(_next_current_path)

if(NOT _sdl3_net_framework_path)
    message(FATAL_ERROR "Could not find SDL3_net.framework root from ${CMAKE_CURRENT_LIST_DIR}")
endif()

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL3_net-target.cmake files.

if(NOT TARGET SDL3_net::SDL3_net-shared)
    add_library(SDL3_net::SDL3_net-shared SHARED IMPORTED)
    set_target_properties(SDL3_net::SDL3_net-shared
        PROPERTIES
            FRAMEWORK "TRUE"
            IMPORTED_LOCATION "${_sdl3_net_framework_path}/SDL3_net"
            COMPATIBLE_INTERFACE_BOOL "SDL3_SHARED"
            INTERFACE_SDL3_SHARED "ON"
            COMPATIBLE_INTERFACE_STRING "SDL_VERSION"
            INTERFACE_SDL_VERSION "SDL3"
    )
endif()
set(SDL3_net_SDL3_net-shared_FOUND TRUE)

set(SDL3_net_SDL3_net-static FALSE)

unset(_sdl3_net_framework_path)

if(SDL3_net_SDL3_net-shared_FOUND)
    set(SDL3_net_SDL3_net_FOUND TRUE)
endif()

function(_sdl_create_target_alias_compat NEW_TARGET TARGET)
    if(CMAKE_VERSION VERSION_LESS "3.18")
        # Aliasing local targets is not supported on CMake < 3.18, so make it global.
        add_library(${NEW_TARGET} INTERFACE IMPORTED)
        set_target_properties(${NEW_TARGET} PROPERTIES INTERFACE_LINK_LIBRARIES "${TARGET}")
    else()
        add_library(${NEW_TARGET} ALIAS ${TARGET})
    endif()
endfunction()

# Make sure SDL3_net::SDL3_net always exists
if(NOT TARGET SDL3_net::SDL3_net)
    if(TARGET SDL3_net::SDL3_net-shared)
        _sdl_create_target_alias_compat(SDL3_net::SDL3_net SDL3_net::SDL3_net-shared)
    endif()
endif()

check_required_components(SDL3_net)
