#
# Rosalie's Mupen GUI CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Core REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

find_package(Git REQUIRED)

execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(Config.hpp.in Config.hpp)

add_executable(RMG
    UserInterface/MainWindow.cpp
    UserInterface/RomBrowserWidget.cpp
    UserInterface/NoFocusDelegate.cpp
    UserInterface/UIResources.rc
    UserInterface/UIResources.qrc
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
    Utilities/Logger.cpp
    Utilities/Settings.cpp
    Globals.cpp
    main.cpp
)

target_link_libraries(RMG ${SDL2_LIBRARIES})

target_include_directories(RMG PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

if(UNIX)
    target_link_libraries(RMG dl)
endif(UNIX)

target_link_libraries(RMG Qt5::Widgets)
