#
# RMG-Audio CMakeLists.txt
#
project(RMG-Audio)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Gui Widgets Core REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

set(RMG_AUDIO_SOURCES
    UserInterface/MainDialog.cpp
    UserInterface/MainDialog.ui
    UserInterface/UIResources.qrc
    main.cpp
)

add_library(RMG-Audio SHARED ${RMG_AUDIO_SOURCES})

set_target_properties(RMG-Audio PROPERTIES PREFIX "")

target_link_libraries(RMG-Audio
    RMG-Core
    ${SDL2_LIBRARIES}
    Qt5::Gui Qt5::Widgets
)

target_include_directories(RMG-Audio PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${SDL2_INCLUDE_DIRS}
)
