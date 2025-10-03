#
# Rosalie's Mupen GUI CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Gui Widgets Core REQUIRED)

if (NOT WIN32)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
endif(NOT WIN32)

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

if (DRAG_DROP)
    add_definitions(-DDRAG_DROP)
endif(DRAG_DROP)

if (NETPLAY)
    find_package(Qt6 COMPONENTS WebSockets REQUIRED)
    add_definitions(-DNETPLAY)
endif(NETPLAY)

find_package(SDL3 REQUIRED)

set(RMG_SOURCES
    UserInterface/MainWindow.cpp
    UserInterface/MainWindow.ui
    UserInterface/Widget/RomBrowser/RomBrowserWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserListViewWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserGridViewWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserLoadingWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserEmptyWidget.cpp
    UserInterface/Widget/RomBrowser/RomBrowserEmptyWidget.ui
    UserInterface/Widget/RomBrowser/RomBrowserSearchWidget.cpp
    UserInterface/Widget/Render/DummyWidget.cpp
    UserInterface/Widget/Render/OGLWidget.cpp
    UserInterface/Widget/Render/VKWidget.cpp
    UserInterface/Widget/KeybindButton.cpp
    UserInterface/Dialog/SettingsDialog.cpp
    UserInterface/Dialog/SettingsDialog.ui
    UserInterface/Dialog/Cheats/CheatsCommon.cpp
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
    Utilities/QtKeyToSdl3Key.cpp
    Utilities/QtMessageBox.cpp
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

if (NETPLAY)
    list(APPEND RMG_SOURCES
        UserInterface/Dialog/Netplay/NetplayCommon.cpp
        UserInterface/Dialog/Netplay/CreateNetplaySessionDialog.cpp
        UserInterface/Dialog/Netplay/CreateNetplaySessionDialog.ui
        UserInterface/Dialog/Netplay/NetplaySessionBrowserDialog.cpp
        UserInterface/Dialog/Netplay/NetplaySessionBrowserDialog.ui
        UserInterface/Dialog/Netplay/NetplaySessionDialog.cpp
        UserInterface/Dialog/Netplay/NetplaySessionDialog.ui
        UserInterface/Dialog/Netplay/NetplaySessionPasswordDialog.cpp
        UserInterface/Dialog/Netplay/NetplaySessionPasswordDialog.ui
        UserInterface/Widget/Netplay/NetplaySessionBrowserWidget.cpp
        UserInterface/Widget/Netplay/NetplaySessionBrowserLoadingWidget.cpp
        UserInterface/Widget/Netplay/NetplaySessionBrowserEmptyWidget.cpp
        UserInterface/Widget/Netplay/NetplaySessionBrowserEmptyWidget.ui
        UserInterface/Widget/Netplay/CreateNetplaySessionWidget.cpp
        UserInterface/Widget/Netplay/CreateNetplaySessionEmptyWidget.cpp
        UserInterface/Widget/Netplay/CreateNetplaySessionEmptyWidget.ui
    )
endif(NETPLAY)

set_source_files_properties(${IMGUI_SOURCES} PROPERTIES GENERATED TRUE)
list(APPEND RMG_SOURCES ${IMGUI_SOURCES})

if (CMAKE_BUILD_TYPE MATCHES Release)
    add_executable(RMG WIN32 ${RMG_SOURCES})
else()
    add_executable(RMG ${RMG_SOURCES})
endif()

target_link_libraries(RMG 
    RMG-Core
    imgui
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
    ${IMGUI_DIR}
)

target_link_libraries(RMG Qt6::Gui Qt6::Widgets)

if (NOT WIN32)
    target_link_libraries(RMG Qt6::DBus)
endif(NOT WIN32)

if (UPDATER)
    target_link_libraries(RMG Qt6::Network)
endif(UPDATER)

if (NETPLAY)
    target_link_libraries(RMG Qt6::WebSockets)
endif(NETPLAY)

