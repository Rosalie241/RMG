#
# Rosalie's Mupen GUI CMakeLists.txt
#
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets Core REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(RMG
    UserInterface/MainWindow.cpp
    UserInterface/RomBrowser.cpp
    Mupen/Mupen.cpp
    Mupen/VidExt.cpp
    Logger.cpp
    main.cpp
)

if(UNIX)
    target_link_libraries(RMG dl ${SDL2_LIBRARIES})
    target_include_directories(RMG PRIVATE ${SDL2_INCLUDE_DIRS})
endif(UNIX)

target_link_libraries(RMG Qt5::Widgets)
