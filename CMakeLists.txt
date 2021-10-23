#
# Rosalie's Mupen GUI CMakeLists.txt
#
cmake_minimum_required(VERSION 3.15)

project(RMG)

find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_INSTALL_PREFIX "")
set(INSTALL_PATH Bin/${CMAKE_BUILD_TYPE})

add_subdirectory(Source/RMG-Core)
add_subdirectory(Source/RMG)
install(TARGETS RMG
    DESTINATION ${INSTALL_PATH}
)

if (WIN32)
    add_subdirectory(Source/Installer)
    
    add_custom_target(bundle_dependencies
        COMMAND "${CMAKE_SOURCE_DIR}/Source/Script/BundleDependencies.sh" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/RMG.exe" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/" "/mingw64/bin"
	)
endif()

add_subdirectory(Source/3rdParty)
install(FILES ${MUPEN64PLUSCORE_LIB}
    DESTINATION ${INSTALL_PATH}/Core
)
install(FILES ${MUPEN64PLUSCORE_INI} ${MUPEN64PLUSCORE_CHT} Data/font.ttf
    DESTINATION ${INSTALL_PATH}/Data
)
install(FILES Config/stylesheet.qss
    DESTINATION ${INSTALL_PATH}/Config
)
install(FILES ${MUPEN64PLUS_PLUGIN_AUDIO}
    DESTINATION ${INSTALL_PATH}/Plugin/Audio
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP}
    DESTINATION ${INSTALL_PATH}/Plugin/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT}
    DESTINATION ${INSTALL_PATH}/Plugin/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_GFX}
    DESTINATION ${INSTALL_PATH}/Plugin/GFX
)
