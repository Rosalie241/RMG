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
pkg_check_modules(LIBUSB1 REQUIRED libusb-1.0)

set(RMG_INPUT_GCA_SOURCES
    UserInterface/MainDialog.cpp
    UserInterface/MainDialog.ui
    main.cpp
)

add_library(RMG-Input-GCA SHARED ${RMG_INPUT_GCA_SOURCES})

target_link_libraries(RMG-Input RMG-Core)

target_include_directories(RMG-Input-GCA PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${LIBUSB1_INCLUDE_DIRS}
)

target_link_libraries(RMG-Input-GCA 
    RMG-Core 
    ${LIBUSB1_LIBRARIES} 
    Qt6::Gui Qt6::Widgets Qt6::Svg 
    atomic # needed by GCC due to std::atomic usage
)
