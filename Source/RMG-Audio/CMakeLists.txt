#
# RMG-Audio CMakeLists.txt
#
project(RMG-Audio)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Gui Widgets Core REQUIRED)
find_package(SDL3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SPEEX REQUIRED speexdsp)
pkg_check_modules(SAMPLERATE REQUIRED samplerate)

set(RMG_AUDIO_SOURCES
    UserInterface/MainDialog.cpp
    UserInterface/MainDialog.ui
    UserInterface/UIResources.qrc
    Resamplers/trivial.cpp
    Resamplers/src.cpp
    Resamplers/speex.cpp
    Resamplers/resamplers.cpp
    sdl_backend.cpp
    main.cpp
)

add_library(RMG-Audio SHARED ${RMG_AUDIO_SOURCES})

set_target_properties(RMG-Audio PROPERTIES PREFIX "")

target_link_libraries(RMG-Audio
    RMG-Core
    SDL3::SDL3
    ${SPEEX_LIBRARIES}
    ${SAMPLERATE_LIBRARIES}
    Qt6::Gui Qt6::Widgets
)

target_include_directories(RMG-Audio PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${SPEEX_INCLUDE_DIRS}
    ${SAMPLERATE_INCLUDE_DIRS}
)