#
# Rosalie's Mupen GUI CMakeLists.txt
#
cmake_minimum_required(VERSION 3.15)

option(PORTABLE_INSTALL "Portable Installation" ON)
option(NO_GIT_CLONE     "Disables git clone usage for 3rdParty dependencies" OFF)

project(RMG)

find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT PORTABLE_INSTALL AND (WIN32 OR MSYS))
    message(WARNING "Not-Portable build not supported on windows, re-enabling PORTABLE_INSTALL!")
    set(PORTABLE_INSTALL ON)
endif()

if (PORTABLE_INSTALL)
    set(CMAKE_INSTALL_PREFIX "")
    set(INSTALL_PATH "Bin/${CMAKE_BUILD_TYPE}")
    set(RMG_INSTALL_PATH "${INSTALL_PATH}")
    set(CORE_INSTALL_PATH "${INSTALL_PATH}/Core")
    set(PLUGIN_INSTALL_PATH "${INSTALL_PATH}/Plugin")
    set(DATA_INSTALL_PATH "${INSTALL_PATH}/Data")
    set(INSTALL_DESKTOP_FILE OFF)
else()
    set(RMG_INSTALL_PATH "bin")
    set(CORE_INSTALL_PATH "lib/RMG/Core")
    set(PLUGIN_INSTALL_PATH "lib/RMG/Plugin")
    set(DATA_INSTALL_PATH "share/RMG")
    set(INSTALL_DESKTOP_FILE ON)
    set(APPDATA_INSTALL_PATH "share/metainfo/")
    set(DESKTOP_INSTALL_PATH "share/applications")
    set(ICON_INSTALL_PATH "share/icons/hicolor/scalable/apps/")
endif()

add_subdirectory(Source/RMG-Core)
add_subdirectory(Source/RMG)
add_subdirectory(Source/RMG-Audio)
install(TARGETS RMG
    DESTINATION ${RMG_INSTALL_PATH}
)
install(TARGETS RMG-Audio
    DESTINATION ${PLUGIN_INSTALL_PATH}/Audio
)

if (WIN32)
    add_subdirectory(Source/Installer)
    
    add_custom_target(bundle_dependencies
        COMMAND "${CMAKE_SOURCE_DIR}/Source/Script/BundleDependencies.sh" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/RMG.exe" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/" "/mingw64/bin"
	)
endif()

add_subdirectory(Source/3rdParty)
install(FILES ${MUPEN64PLUSCORE_LIB}
    DESTINATION ${CORE_INSTALL_PATH}
)
install(FILES ${MUPEN64PLUSCORE_INI} ${MUPEN64PLUSCORE_CHT} Data/font.ttf Data/stylesheet.qss
    DESTINATION ${DATA_INSTALL_PATH}
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP_HLE}
    DESTINATION ${PLUGIN_INSTALL_PATH}/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP_PARALLEL}
    DESTINATION ${PLUGIN_INSTALL_PATH}/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT_QT}
    DESTINATION ${PLUGIN_INSTALL_PATH}/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT_RAPHNET}
    DESTINATION ${PLUGIN_INSTALL_PATH}/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT_GCA}
    DESTINATION ${PLUGIN_INSTALL_PATH}/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_GFX_GLIDEN64}
    DESTINATION ${PLUGIN_INSTALL_PATH}/GFX
)
install(FILES ${MUPEN64PLUS_PLUGIN_GFX_PARALLEL}
    DESTINATION ${PLUGIN_INSTALL_PATH}/GFX
)

if (INSTALL_DESKTOP_FILE)
    install(FILES Package/com.github.Rosalie241.RMG.desktop
        DESTINATION ${DESKTOP_INSTALL_PATH}
    )
    install(FILES Package/com.github.Rosalie241.RMG.svg
        DESTINATION ${ICON_INSTALL_PATH}
    )
    install(FILES Package/com.github.Rosalie241.RMG.appdata.xml
        DESTINATION ${APPDATA_INSTALL_PATH}
    )
endif(INSTALL_DESKTOP_FILE)
