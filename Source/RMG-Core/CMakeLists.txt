#
# RMG-Core CMakeLists.txt
#
project(RMG-Core)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
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
    m64p/PluginApi.cpp
    CachedRomHeaderAndSettings.cpp
    ConvertStringEncoding.cpp
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
    Settings.cpp
    Archive.cpp
    Library.cpp
    Netplay.cpp
    Plugins.cpp
    Version.cpp
    Cheats.cpp
    String.cpp
    Volume.cpp
    VidExt.cpp
    Video.cpp
    Error.cpp
    Core.cpp
    File.cpp
    Key.cpp
    Rom.cpp
)

if (NETPLAY)
    add_definitions(-DNETPLAY)
endif(NETPLAY)

if (PORTABLE_INSTALL)
    add_definitions(-DPORTABLE_INSTALL)
endif(PORTABLE_INSTALL)

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

target_include_directories(RMG-Core PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${MINIZIP_INCLUDE_DIRS}
)

set_target_properties(RMG-Core PROPERTIES CXX_VISIBILITY_PRESET hidden)
