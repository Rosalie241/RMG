# SDL2_net CMake configuration file:
# This file is meant to be placed in a cmake subfolder of SDL2_net-devel-2.x.y-VC

include(FeatureSummary)
set_package_properties(SDL2_net PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_net/"
    DESCRIPTION "SDL_net is an example portable network library for use with SDL."
)
cmake_minimum_required(VERSION 3.0)

set(SDL2_net_FOUND TRUE)

if(CMAKE_SIZEOF_VOID_P STREQUAL "4")
    set(_sdl_arch_subdir "x86")
elseif(CMAKE_SIZEOF_VOID_P STREQUAL "8")
    set(_sdl_arch_subdir "x64")
else()
    unset(_sdl_arch_subdir)
    set(SDL2_net_FOUND FALSE)
    return()
endif()

set(_sdl2net_incdir       "${CMAKE_CURRENT_LIST_DIR}/../include")
set(_sdl2net_library      "${CMAKE_CURRENT_LIST_DIR}/../lib/${_sdl_arch_subdir}/SDL2_net.lib")
set(_sdl2net_dll          "${CMAKE_CURRENT_LIST_DIR}/../lib/${_sdl_arch_subdir}/SDL2_net.dll")

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL2_net-target.cmake files.

if(NOT TARGET SDL2_net::SDL2_net)
    add_library(SDL2_net::SDL2_net SHARED IMPORTED)
    set_target_properties(SDL2_net::SDL2_net
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl2net_incdir}"
            IMPORTED_IMPLIB "${_sdl2net_library}"
            IMPORTED_LOCATION "${_sdl2net_dll}"
            COMPATIBLE_INTERFACE_BOOL "SDL2_SHARED"
            INTERFACE_SDL2_SHARED "ON"
    )
endif()

unset(_sdl_arch_subdir)
unset(_sdl2net_incdir)
unset(_sdl2net_library)
unset(_sdl2net_dll)
