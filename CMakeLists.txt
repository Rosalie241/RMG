#
# Rosalie's Mupen GUI CMakeLists.txt
#
cmake_minimum_required(VERSION 3.15)

project(RMG)

set(CMAKE_INSTALL_PREFIX "")

find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_subdirectory(Source/RMG)
install(TARGETS RMG
    DESTINATION rmg
)

if (WIN32)
    add_subdirectory(Source/Installer)

	add_custom_target(bundle_dependencies
		COMMAND "${CMAKE_SOURCE_DIR}/Source/Script/BundleDependencies.sh" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/rmg/RMG.exe" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/rmg" "/mingw64/bin"
	)
endif()

add_subdirectory(Source/3rdParty)
install(FILES ${MUPEN64PLUSCORE_LIB}
    DESTINATION rmg/Core
)
install(FILES ${MUPEN64PLUSCORE_INI} ${MUPEN64PLUSCORE_CHT}
    DESTINATION rmg/Data
)
install(FILES Config/stylesheet.qss
    DESTINATION rmg/Config
)
install(FILES ${MUPEN64PLUS_PLUGIN_AUDIO}
    DESTINATION rmg/Plugin/Audio
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP}
    DESTINATION rmg/Plugin/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT}
    DESTINATION rmg/Plugin/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_GFX}
    DESTINATION rmg/Plugin/GFX
)
