cmake_minimum_required(VERSION 3.9)

option(BUILD_MUPEN64PLUS "Enables build of mupen64plus version" ON)
option(BUILD_PROJECT64   "Enables build of project64 version" WIN32)
option(GLES "Set to ON to use OpenGL ES 3.0 renderer instead of OpenGL 3.3 core")
option(USE_QT5 "Enables Qt5 instead of Qt6 (will be removed in the future)" OFF)

project(angrylion-plus)

include(GNUInstallDirs)

set(OpenGL_GL_PREFERENCE GLVND)

if (BUILD_PROJECT64 AND NOT WIN32)
    message(WARNING "BUILD_PROJECT64 is ON but not supported on current platform, disabling option")
    set(BUILD_PROJECT64 OFF)
endif()

if(GLES)
    message("OpenGL ES 3.0 renderer enabled")
    add_definitions(-DGLES)
endif(GLES)

if(ANDROID)
    add_definitions(-DANDROID)
endif()

if (USE_QT5)
    set(QT_VERSION Qt5)
else(USE_QT5)
    set(QT_VERSION Qt6)
endif(USE_QT5)

# set policy CMP0042 for MacOS X
set(CMAKE_MACOSX_RPATH 1)

# check for INTERPROCEDURAL_OPTIMIZATION support
cmake_policy(SET CMP0069 NEW)

include(CheckIPOSupported)
check_ipo_supported(RESULT ENABLE_IPO)
if(ENABLE_IPO)
    message("Interprocedural optimizations enabled")
endif(ENABLE_IPO)

# C++14 is required for the Parallel utility class
set(CMAKE_CXX_STANDARD 14)

# disable warnings to use unportable secure file IO
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

# default to release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if (NOT ANDROID)
    find_package(OpenGL REQUIRED)
endif()

set(PATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# RDP core library
set(PATH_CORE "${PATH_SRC}/core")

# run script to generate version.h
set(PATH_VERSION "${PATH_CORE}/version.h")

add_custom_command(
    OUTPUT ${PATH_VERSION}
    COMMAND
        ${CMAKE_COMMAND} -DPATH_VERSION=${PATH_VERSION}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake
    COMMENT "Generate Git version"
)

# static core library
file(GLOB SOURCES_CORE "${PATH_CORE}/*.c" "${PATH_CORE}/*.cpp")

if (ANDROID)
    find_library( # Defines the name of the path variable that stores the
        # location of the NDK library.
        OPENGL_LIBRARIES

        # Specifies the name of the NDK library that
        # CMake needs to locate.
        GLESv3 )
endif()

add_library(alp-core STATIC ${SOURCES_CORE} ${PATH_VERSION})

# output library
set(PATH_OUTPUT "${PATH_SRC}/output")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(${QT_VERSION} COMPONENTS Gui Widgets Core REQUIRED)

file(GLOB SOURCES_OUTPUT "${PATH_OUTPUT}/*.c" "${PATH_OUTPUT}/*.cpp")

add_library(alp-output STATIC ${SOURCES_OUTPUT})

if(MINGW)
    # link libgcc/libstdc++ statically, fixes cryptic "_ZNSt13runtime_errorC1EPKc" error
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
else(MINGW)
    # set PIC option for non-MinGW targets
    set_target_properties(alp-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(alp-output PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif(MINGW)

# set IPO option, if supported
if(ENABLE_IPO AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -O3")
endif()

include_directories(${PATH_SRC})

# Project64 GFX Plugin (Windows only)
if(BUILD_PROJECT64)
    set(NAME_PLUGIN_ZILMAR ${CMAKE_PROJECT_NAME})
    set(PATH_PLUGIN_ZILMAR "${PATH_SRC}/plugin/zilmar")

    enable_language(RC)
    file(GLOB SOURCES_PLUGIN_ZILMAR "${PATH_PLUGIN_ZILMAR}/*.c" "${PATH_PLUGIN_ZILMAR}/*.rc")
    add_library(${NAME_PLUGIN_ZILMAR} SHARED ${SOURCES_PLUGIN_ZILMAR})

    set_target_properties(${NAME_PLUGIN_ZILMAR} PROPERTIES PREFIX "")

    target_link_libraries(${NAME_PLUGIN_ZILMAR} alp-core alp-output shlwapi ${OPENGL_LIBRARIES})
endif(BUILD_PROJECT64)

# Mupen64Plus GFX plugin
if(BUILD_MUPEN64PLUS)
    set(NAME_PLUGIN_M64P "mupen64plus-video-${CMAKE_PROJECT_NAME}")
    
    set(PATH_PLUGIN_M64P "${PATH_SRC}/plugin/mupen64plus")
    
    file(GLOB SOURCES_PLUGIN_M64P "${PATH_PLUGIN_M64P}/*.c" "${PATH_PLUGIN_M64P}/*.cpp" "${PATH_PLUGIN_M64P}/UserInterface/MainDialog.cpp" "${PATH_PLUGIN_M64P}/UserInterface/MainDialog.ui")
    add_library(${NAME_PLUGIN_M64P} SHARED ${SOURCES_PLUGIN_M64P})
    
    if(NOT ANDROID)
        set_target_properties(${NAME_PLUGIN_M64P} PROPERTIES PREFIX "")
    endif()

    target_link_libraries(${NAME_PLUGIN_M64P} alp-core alp-output ${QT_VERSION}::Gui ${QT_VERSION}::Widgets ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_LIBRARIES})
    
    if(UNIX AND NOT APPLE AND NOT ANDROID)
        install(TARGETS ${NAME_PLUGIN_M64P}
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/mupen64plus"
        )
    endif()
endif(BUILD_MUPEN64PLUS)
