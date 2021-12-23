#
# Rosalie's Mupen GUI CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Gui Widgets Core REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

configure_file(Config.hpp.in Config.hpp)

set(RMG_SOURCES
    UserInterface/MainWindow.cpp
    UserInterface/Widget/RomBrowserWidget.cpp
    UserInterface/Widget/OGLWidget.cpp
    UserInterface/Widget/KeyBindButton.cpp
    UserInterface/Dialog/SettingsDialog.cpp
    UserInterface/Dialog/SettingsDialog.ui
    UserInterface/Dialog/SettingsDialog.qrc
    UserInterface/NoFocusDelegate.cpp
    UserInterface/EventFilter.cpp
    UserInterface/UIResources.rc
    UserInterface/UIResources.qrc
    Thread/RomSearcherThread.cpp
    Thread/EmulationThread.cpp
    Utilities/QtKeyToSdl2Key.cpp
    Callbacks.cpp
    VidExt.cpp
    main.cpp
)

if (CMAKE_BUILD_TYPE MATCHES Release)
    add_executable(RMG WIN32 ${RMG_SOURCES})
else()
    add_executable(RMG ${RMG_SOURCES})
endif()

target_link_libraries(RMG 
    RMG-Core
    ${SDL2_LIBRARIES}
)

target_include_directories(RMG PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(RMG Qt5::Gui Qt5::Widgets)
