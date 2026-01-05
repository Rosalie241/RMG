#
# RMG-Input CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# disable lib prefix
set(CMAKE_SHARED_LIBRARY_PREFIX "")

find_package(Qt6 COMPONENTS Gui Widgets Core Svg REQUIRED)

find_package(SDL3 REQUIRED)

set(RMG_INPUT_SOURCES
    UserInterface/Layout/AlignCenterLayout.cpp
    UserInterface/Widget/ControllerWidget.ui
    UserInterface/Widget/ControllerWidget.cpp
    UserInterface/Widget/ControllerImageWidget.cpp
    UserInterface/Widget/HotkeyButton.cpp
    UserInterface/Widget/MappingButton.cpp
    UserInterface/Widget/AddMappingButton.cpp
    UserInterface/Widget/RemoveMappingButton.cpp
    UserInterface/EventFilter.cpp
    UserInterface/OptionsDialog.ui
    UserInterface/OptionsDialog.cpp
    UserInterface/HotkeysDialog.ui
    UserInterface/HotkeysDialog.cpp
    UserInterface/MainDialog.ui
    UserInterface/MainDialog.cpp
    UserInterface/UIResources.qrc
    UserInterface/UICommon.cpp
    Utilities/Sdl3ButtonLabelToString.cpp
    Utilities/InputProfileDB.cpp
    Utilities/QtKeyToSdl3Key.cpp
    Thread/SDLThread.cpp
    Thread/HotkeysThread.cpp
    main.cpp
)

if (VRU)
    list(APPEND RMG_INPUT_SOURCES
        VRU.cpp
        VRUwords.cpp
    )
    add_definitions(-DVRU)
endif(VRU)

# Enable Kaillera netplay support
add_definitions(-DNETPLAY)

add_library(RMG-Input SHARED ${RMG_INPUT_SOURCES})

target_link_libraries(RMG-Input RMG-Core SDL3::SDL3)

target_include_directories(RMG-Input PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

target_link_libraries(RMG-Input Qt6::Gui Qt6::Widgets Qt6::Svg)
