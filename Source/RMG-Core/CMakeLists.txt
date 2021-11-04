#
# RMG-Core CMakeLists.txt
#
project(RMG-Core)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(MINIZIP REQUIRED minizip)

set(RMG_CORE_SOURCES
    m64p/Api.cpp
    m64p/CoreApi.cpp
    m64p/ConfigApi.cpp
    m64p/PluginApi.cpp
    Settings/Settings.cpp
    Emulation.cpp
    SaveState.cpp
    Plugins.cpp
    VidExt.cpp
    Video.cpp
    Error.cpp
    Core.cpp
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

add_library(RMG-Core STATIC ${RMG_CORE_SOURCES})

target_link_libraries(RMG-Core
    ${MINIZIP_LIBRARIES}
)

target_include_directories(RMG-Core PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MINIZIP_INCLUDE_DIRS}
)
