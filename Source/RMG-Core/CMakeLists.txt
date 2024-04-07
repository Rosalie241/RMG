#
# RMG-Core CMakeLists.txt
#
project(RMG-Core)

set(CMAKE_CXX_STANDARD 20)

# RMG-Core needs LTO disabled for windows
# else linking fails
if (WIN32)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif(WIN32)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(MINIZIP REQUIRED minizip)
if (WIN32)
    pkg_check_modules(ICONV REQUIRED iconv)
endif(WIN32)

if (NOT PORTABLE_INSTALL)
    include(GNUInstallDirs)
endif(NOT PORTABLE_INSTALL)

configure_file(Config.hpp.in Config.hpp)

set(RMG_CORE_SOURCES
    m64p/Api.cpp
    m64p/CoreApi.cpp
    m64p/ConfigApi.cpp
    m64p/DebugApi.cpp
    m64p/PluginApi.cpp
    CachedRomHeaderAndSettings.cpp
    ConvertStringEncoding.cpp
    Settings/Settings.cpp
    SpeedLimiter.cpp
    SpeedFactor.cpp
    RomSettings.cpp
    Directories.cpp
    MediaLoader.cpp
    Screenshot.cpp
    RomHeader.cpp
    Emulation.cpp
    SaveState.cpp
    Callback.cpp
    Plugins.cpp
    Version.cpp
    Cheats.cpp
    Volume.cpp
    VidExt.cpp
    Video.cpp
    Error.cpp
    Unzip.cpp
    Core.cpp
    Key.cpp
    Rom.cpp
)

if (WIN32)
    list(APPEND RMG_CORE_SOURCES 
        osal/osal_dynlib_win32.cpp
        osal/osal_files_win32.cpp
    )
else(WIN32)
    list(APPEND RMG_CORE_SOURCES 
		osal/osal_dynlib_unix.cpp
        osal/osal_files_unix.cpp
    )
endif(WIN32)

if (DISCORD_RPC)
    list(APPEND RMG_CORE_SOURCES 
        DiscordRpc.cpp
    )
    add_definitions(-DDISCORD_RPC)
endif(DISCORD_RPC)

if (PORTABLE_INSTALL)
    add_definitions(-DPORTABLE_INSTALL)
endif(PORTABLE_INSTALL)

if (USE_LIBFMT)
    add_definitions(-DUSE_LIBFMT)
endif(USE_LIBFMT)

add_library(RMG-Core SHARED ${RMG_CORE_SOURCES})

if(UNIX)
    target_link_libraries(RMG-Core dl)
endif(UNIX)

if(WIN32)
    target_link_libraries(RMG-Core wsock32 ws2_32 ${ICONV_LIBRARIES})
endif(WIN32)

target_link_libraries(RMG-Core
    ${MINIZIP_LIBRARIES}
    lzma
)

if (DISCORD_RPC)
    target_link_libraries(RMG-Core
        ${DISCORD_RPC_LIB}
    )
    add_dependencies(RMG-Core discord-rpc)
endif(DISCORD_RPC)

target_include_directories(RMG-Core PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/fmt/include/
    ${MINIZIP_INCLUDE_DIRS}
)
