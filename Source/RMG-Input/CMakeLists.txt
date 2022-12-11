#
# RMG-Input CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# disable lib prefix
set(CMAKE_SHARED_LIBRARY_PREFIX "")

find_package(Qt6 COMPONENTS Gui Widgets Core Svg REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

set(RMG_INPUT_SOURCES
    UserInterface/Widget/ControllerWidget.ui
    UserInterface/Widget/ControllerWidget.cpp
    UserInterface/Widget/ControllerImageWidget.cpp
    UserInterface/Widget/CustomButton.cpp
    UserInterface/EventFilter.cpp
    UserInterface/OptionsDialog.ui
    UserInterface/OptionsDialog.cpp
    UserInterface/MainDialog.ui
    UserInterface/MainDialog.cpp
    UserInterface/UIResources.qrc
    Utilities/QtKeyToSdl2Key.cpp
    Utilities/InputDevice.cpp
    Thread/SDLThread.cpp
    main.cpp
)

add_library(RMG-Input SHARED ${RMG_INPUT_SOURCES})

target_link_libraries(RMG-Input RMG-Core ${SDL2_LIBRARIES})

target_include_directories(RMG-Input PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../ 
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(RMG-Input Qt6::Gui Qt6::Widgets Qt6::Svg)
