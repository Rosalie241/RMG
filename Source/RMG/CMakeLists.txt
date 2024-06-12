#
# Rosalie's Mupen GUI CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Gui Widgets Core REQUIRED)

if (PORTABLE_INSTALL)
    add_definitions(-DPORTABLE_INSTALL)
endif(PORTABLE_INSTALL)

if (UPDATER)
    find_package(Qt6 COMPONENTS Network REQUIRED)
    add_definitions(-DUPDATER)
endif(UPDATER)

if (APPIMAGE_UPDATER)
    add_definitions(-DAPPIMAGE_UPDATER)
endif(APPIMAGE_UPDATER)

if (DISCORD_RPC)
    add_definitions(-DDISCORD_RPC)
endif(DISCORD_RPC)

if (DRAG_DROP)
    add_definitions(-DDRAG_DROP)
endif(DRAG_DROP)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

set(RMG_SOURCES
    UserInterface/MainWindow.cpp
    UserInterface/MainWindow.ui
    UserInterface/Widget/RomBrowser/RomBrowserWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserListViewWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserGridViewWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserLoadingWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserEmptyWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserEmptyWidget.ui
    UserInterface/Widget/Render/DummyWidget.cpp
    UserInterface/Widget/Render/OGLWidget.cpp
    UserInterface/Widget/Render/VKWidget.cpp
    UserInterface/Widget/KeybindButton.cpp
    UserInterface/Dialog/SettingsDialog.cpp
    UserInterface/Dialog/SettingsDialog.ui
    UserInterface/Dialog/Cheats/CheatsDialog.cpp
    UserInterface/Dialog/Cheats/CheatsDialog.ui
    UserInterface/Dialog/Cheats/AddCheatDialog.cpp
    UserInterface/Dialog/Cheats/AddCheatDialog.ui
    UserInterface/Dialog/Cheats/ChooseCheatOptionDialog.cpp
    UserInterface/Dialog/Cheats/ChooseCheatOptionDialog.ui
    UserInterface/Dialog/RomInfoDialog.cpp
    UserInterface/Dialog/RomInfoDialog.ui
    UserInterface/Dialog/AboutDialog.cpp
    UserInterface/Dialog/AboutDialog.ui
    UserInterface/Dialog/LogDialog.cpp
    UserInterface/Dialog/LogDialog.ui
    UserInterface/NoFocusDelegate.cpp
    UserInterface/EventFilter.cpp
    UserInterface/UIResources.rc
    UserInterface/UIResources.qrc
    Thread/RomSearcherThread.cpp
    Thread/EmulationThread.cpp
    Utilities/QtKeyToSdl2Key.cpp
    OnScreenDisplay.cpp
    Callbacks.cpp
    VidExt.cpp
    main.cpp
)

if (UPDATER)
    list(APPEND RMG_SOURCES
        UserInterface/Dialog/Update/UpdateDialog.cpp
        UserInterface/Dialog/Update/UpdateDialog.ui
        UserInterface/Dialog/Update/DownloadUpdateDialog.cpp
        UserInterface/Dialog/Update/DownloadUpdateDialog.ui
        UserInterface/Dialog/Update/InstallUpdateDialog.cpp
        UserInterface/Dialog/Update/InstallUpdateDialog.ui
    )
endif(UPDATER)

set_source_files_properties(${IMGUI_SOURCES} PROPERTIES GENERATED TRUE)
list(APPEND RMG_SOURCES ${IMGUI_SOURCES})

if (CMAKE_BUILD_TYPE MATCHES Release)
    add_executable(RMG WIN32 ${RMG_SOURCES})
else()
    add_executable(RMG ${RMG_SOURCES})
endif()

add_dependencies(RMG 
    imgui
)

target_link_libraries(RMG 
    RMG-Core
)

# needed for dynamically linked RMG-Core
if (PORTABLE_INSTALL)
    set_target_properties(RMG PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
endif(PORTABLE_INSTALL)

target_include_directories(RMG PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${SDL2_INCLUDE_DIRS}
    ${IMGUI_DIR}
)

target_link_libraries(RMG Qt6::Gui Qt6::Widgets)

if (UPDATER)
    target_link_libraries(RMG Qt6::Network)
endif(UPDATER)

