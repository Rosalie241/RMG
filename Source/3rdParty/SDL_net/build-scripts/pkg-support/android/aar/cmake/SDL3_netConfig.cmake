# SDL CMake configuration file:
# This file is meant to be placed in lib/cmake/SDL3_net subfolder of a reconstructed Android SDL3_net SDK

cmake_minimum_required(VERSION 3.0...3.28)

include(FeatureSummary)
set_package_properties(SDL3_net PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_net/"
    DESCRIPTION "SDL_net is a simple, cross-platform wrapper over sockets"
)

# Copied from `configure_package_config_file`
macro(set_and_check _var _file)
    set(${_var} "${_file}")
    if(NOT EXISTS "${_file}")
        message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
    endif()
endmacro()

# Copied from `configure_package_config_file`
macro(check_required_components _NAME)
    foreach(comp ${${_NAME}_FIND_COMPONENTS})
        if(NOT ${_NAME}_${comp}_FOUND)
            if(${_NAME}_FIND_REQUIRED_${comp})
                set(${_NAME}_FOUND FALSE)
            endif()
        endif()
    endforeach()
endmacro()

set(SDL3_net_FOUND TRUE)

if(SDL_CPU_X86)
    set(_sdl_arch_subdir "x86")
elseif(SDL_CPU_X64)
    set(_sdl_arch_subdir "x86_64")
elseif(SDL_CPU_ARM32)
    set(_sdl_arch_subdir "armeabi-v7a")
elseif(SDL_CPU_ARM64)
    set(_sdl_arch_subdir "arm64-v8a")
else()
    set(SDL3_net_FOUND FALSE)
    return()
endif()

get_filename_component(_sdl3net_prefix "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(_sdl3net_prefix "${_sdl3net_prefix}/.." ABSOLUTE)
get_filename_component(_sdl3net_prefix "${_sdl3net_prefix}/.." ABSOLUTE)
set_and_check(_sdl3net_prefix          "${_sdl3net_prefix}")
set_and_check(_sdl3net_include_dirs    "${_sdl3net_prefix}/include")

set_and_check(_sdl3net_lib             "${_sdl3net_prefix}/lib/${_sdl_arch_subdir}/libSDL3_net.so")

unset(_sdl_arch_subdir)
unset(_sdl3net_prefix)

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL3_net-target.cmake files.

set(SDL3_net_SDL3_net-shared_FOUND FALSE)
if(EXISTS "${_sdl3net_lib}")
    if(NOT TARGET SDL3_net::SDL3_net-shared)
        add_library(SDL3_net::SDL3_net-shared SHARED IMPORTED)
        set_target_properties(SDL3_net::SDL3_net-shared
            PROPERTIES
                IMPORTED_LOCATION "${_sdl3net_lib}"
                INTERFACE_INCLUDE_DIRECTORIES "${_sdl3net_include_dirs}"
                COMPATIBLE_INTERFACE_BOOL "SDL3_SHARED"
                INTERFACE_SDL3_SHARED "ON"
                COMPATIBLE_INTERFACE_STRING "SDL_VERSION"
                INTERFACE_SDL_VERSION "SDL3"
        )
    endif()
    set(SDL3_net_SDL3_net-shared_FOUND TRUE)
endif()
unset(_sdl3net_include_dirs)
unset(_sdl3net_lib)

set(SDL3_net_SDL3_net-static_FOUND FALSE)

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
