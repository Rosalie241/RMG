# SDL3 CMake configuration file:
# This file is meant to be placed in share/cmake/SDL3_net, next to SDL3_net.xcframework

# INTERFACE_LINK_OPTIONS needs CMake 3.12
cmake_minimum_required(VERSION 3.12...4.0)

include(FeatureSummary)
set_package_properties(SDL3_net PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_net/"
    DESCRIPTION "SDL_net is a portable networking library"
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

macro(_check_target_is_simulator)
    set(src [===[
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR
    int target_is_simulator;
    #endif
    int main(int argc, char *argv[]) { return target_is_simulator; }
    ]===])
    if(CMAKE_C_COMPILER)
        include(CheckCSourceCompiles)
        check_c_source_compiles("${src}" SDL_TARGET_IS_SIMULATOR)
    elseif(CMAKE_CXX_COMPILER)
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("${src}" SDL_TARGET_IS_SIMULATOR)
    else()
        enable_language(C)
        include(CheckCSourceCompiles)
        check_c_source_compiles("${src}" SDL_TARGET_IS_SIMULATOR)
    endif()
endmacro()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    _check_target_is_simulator()
    if(SDL_TARGET_IS_SIMULATOR)
        set(_xcfw_target_subdir "ios-arm64_x86_64-simulator")
    else()
        set(_xcfw_target_subdir "ios-arm64")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    _check_target_is_simulator()
    if(SDL_TARGET_IS_SIMULATOR)
        set(_xcfw_target_subdir "tvos-arm64_x86_64-simulator")
    else()
        set(_xcfw_target_subdir "tvos-arm64")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_xcfw_target_subdir "macos-arm64_x86_64")
else()
    message(WARNING "Unsupported Apple platform (${CMAKE_SYSTEM_NAME}) and broken SDL3_netConfigVersion.cmake")
    set(SDL3_net_FOUND FALSE)
    return()
endif()

# Compute the installation prefix relative to this file.
get_filename_component(_sdl3_net_xcframework_parent_path "${CMAKE_CURRENT_LIST_DIR}" REALPATH)              # /share/cmake/SDL3/
get_filename_component(_sdl3_net_xcframework_parent_path "${_sdl3_net_xcframework_parent_path}" REALPATH)       # /share/cmake/SDL3/
get_filename_component(_sdl3_net_xcframework_parent_path "${_sdl3_net_xcframework_parent_path}" PATH)           # /share/cmake
get_filename_component(_sdl3_net_xcframework_parent_path "${_sdl3_net_xcframework_parent_path}" PATH)           # /share
get_filename_component(_sdl3_net_xcframework_parent_path "${_sdl3_net_xcframework_parent_path}" PATH)           # /
set_and_check(_sdl3_net_xcframework_path "${_sdl3_net_xcframework_parent_path}/SDL3_net.xcframework")         # /SDL3_net.xcframework
set_and_check(_sdl3_net_framework_parent_path "${_sdl3_net_xcframework_path}/${_xcfw_target_subdir}")           # /SDL3_net.xcframework/macos-arm64_x86_64
set_and_check(_sdl3_net_framework_path "${_sdl3_net_framework_parent_path}/SDL3_net.framework")               # /SDL3_net.xcframework/macos-arm64_x86_64/SDL3_net.framework

set(SDL3_net_FOUND TRUE)

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL3_net-target.cmake files.

if(NOT TARGET SDL3_net::SDL3_net-shared)
    add_library(SDL3_net::SDL3_net-shared SHARED IMPORTED)
    # CMake does not automatically add RPATHS when using xcframeworks
    # https://gitlab.kitware.com/cmake/cmake/-/issues/25998
    if(0)  # if(CMAKE_VERSION GREATER_EQUAL "3.28")
        set_target_properties(SDL3_net::SDL3_net-shared
            PROPERTIES
                FRAMEWORK "TRUE"
                IMPORTED_LOCATION "${_sdl3_net_xcframework_path}"
        )
    else()
        set_target_properties(SDL3_net::SDL3_net-shared
            PROPERTIES
                FRAMEWORK "TRUE"
                IMPORTED_LOCATION "${_sdl3_net_framework_path}/SDL3_net"
        )
    endif()
    set_target_properties(SDL3_net::SDL3_net-shared
        PROPERTIES
            COMPATIBLE_INTERFACE_BOOL "SDL3_SHARED"
            INTERFACE_SDL3_SHARED "ON"
            COMPATIBLE_INTERFACE_STRING "SDL_VERSION"
            INTERFACE_SDL_VERSION "SDL3"
    )
endif()
set(SDL3_net_SDL3_net-shared_FOUND TRUE)

set(SDL3_net_SDL3_net-static FALSE)

unset(_sdl3_net_xcframework_parent_path)
unset(_sdl3_net_xcframework_path)
unset(_sdl3_net_framework_parent_path)
unset(_sdl3_net_framework_path)
unset(_sdl3_net_include_dirs)

if(SDL3_net_SDL3_net-shared_FOUND)
    set(SDL3_net_SDL3_net TRUE)
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
