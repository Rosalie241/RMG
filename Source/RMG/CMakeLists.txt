#
# Rosalie's Mupen GUI CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets Core REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(RMG
    UserInterface/MainWindow.cpp
    UserInterface/RomBrowserWidget.cpp
    Thread/RomSearcherThread.cpp
    Thread/EmulationThread.cpp
    M64P/CoreApi.cpp
    M64P/ConfigApi.cpp
    M64P/PluginApi.cpp
    M64P/Api.cpp
    M64P/Wrapper/Config.cpp
    M64P/Wrapper/Core.cpp
    M64P/Wrapper/Plugin.cpp
    M64P/Wrapper/Api.cpp
    Util/Logger.cpp
    Globals.cpp
    main.cpp
)

if(UNIX)
    target_link_libraries(RMG dl)
endif(UNIX)

target_link_libraries(RMG ${SDL2_LIBRARIES})
target_include_directories(RMG PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(RMG Qt5::Widgets)
