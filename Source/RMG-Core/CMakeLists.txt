#
# RMG-Core CMakeLists.txt
#
project(RMG-Core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(MINIZIP REQUIRED minizip)

set(RMG_CORE_SOURCES
    m64p/Api.cpp
    m64p/CoreApi.cpp
    m64p/ConfigApi.cpp
    m64p/PluginApi.cpp
    Settings/Settings.cpp
    SpeedLimiter.cpp
    RomSettings.cpp
    Directories.cpp
    Screenshot.cpp
    RomHeader.cpp
    Emulation.cpp
    SaveState.cpp
    Callback.cpp
    Plugins.cpp
    VidExt.cpp
    Video.cpp
    Error.cpp
    Core.cpp
    Key.cpp
    Rom.cpp
)

if (WIN32 OR MSYS)
    list(APPEND RMG_CORE_SOURCES 
        osal/osal_dynlib_win32.cpp
    )
else()
    list(APPEND RMG_CORE_SOURCES 
		osal/osal_dynlib_unix.cpp
    )
endif()

if (PORTABLE_INSTALL)
add_definitions(-DPORTABLE_INSTALL)
endif(PORTABLE_INSTALL)

add_library(RMG-Core STATIC ${RMG_CORE_SOURCES})

if(UNIX)
    target_link_libraries(RMG-Core dl)
endif(UNIX)

if(WIN32)
    target_link_libraries(RMG-Core wsock32 ws2_32)
endif(WIN32)

target_link_libraries(RMG-Core
    ${MINIZIP_LIBRARIES}
)

target_include_directories(RMG-Core PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MINIZIP_INCLUDE_DIRS}
)
