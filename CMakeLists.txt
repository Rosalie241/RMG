#
# Rosalie's Mupen GUI CMakeLists.txt
#
cmake_minimum_required(VERSION 3.15)

option(PORTABLE_INSTALL "Portable Installation" ON)
option(UPDATER          "Enables updater" ${PORTABLE_INSTALL})
option(APPIMAGE_UPDATER "Enables AppImage updater" OFF)
option(DISCORD_RPC      "Enables Discord Rich Presence" ON)
option(DRAG_DROP        "Enables drag and drop" ON)
option(VRU              "Enables VRU support in RMG-Input" ON)
option(USE_CCACHE       "Enables usage of ccache when ccache has been found" ON)
option(USE_LTO          "Enables building with LTO/IPO when compiler supports it" ON)
option(NO_RUST          "Disables the building of rust subprojects" OFF)

project(RMG)

find_package(Git)
if (GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE GIT_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif(GIT_FOUND)
if (NOT GIT_FOUND OR NOT ${GIT_RESULT} EQUAL 0)
    if (NOT ${GIT_RESULT} EQUAL 0)
        message(WARNING "git failed to retrieve the current revision, falling back to VERSION file!")
    endif()
    file(STRINGS "VERSION" GIT_VERSION)
endif()

if (USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if (CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    else(CCACHE_FOUND)
        set(USE_CCACHE OFF)
    endif(CCACHE_FOUND)
endif(USE_CCACHE)

if (USE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ENABLE_IPO)

    if(ENABLE_IPO)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif(ENABLE_IPO)
endif(USE_LTO)

if (NOT PORTABLE_INSTALL AND (WIN32 OR MSYS))
    message(WARNING "Not-Portable build not supported on windows, re-enabling PORTABLE_INSTALL!")
    set(PORTABLE_INSTALL ON)
endif()

if (NOT PORTABLE_INSTALL AND UPDATER AND NOT APPIMAGE_UPDATER)
    message(WARNING "Not-Portable builds don't support the updater, disabling UPDATER")
    set(UPDATER OFF)
endif()

if (APPIMAGE_UPDATER AND (WIN32 OR MSYS))
    message(WARNING "AppImage updater not supported on windows, disabling APPIMAGE_UPDATER!")
    set(APPIMAGE_UPDATER OFF)
endif()

if (UPDATER AND NOT (WIN32 OR MSYS) AND NOT APPIMAGE_UPDATER)
    message(WARNING "Only AppImage updater is supported on linux, disabling UPDATER!")
    set(UPDATER OFF)
endif()

if (FORCE_XCB AND (WIN32 OR MSYS))
    message(WARNING "XCB Qt platform not supported on windows, disabling FORCE_XCB!")
    set(FORCE_XCB OFF)
endif()

if (PORTABLE_INSTALL)
    set(CMAKE_INSTALL_PREFIX "")
    set(INSTALL_PATH "Bin/${CMAKE_BUILD_TYPE}")
    set(RMG_INSTALL_PATH "${INSTALL_PATH}")
    set(SYSTEM_LIB_INSTALL_PATH "${INSTALL_PATH}")
    set(LIB_INSTALL_PATH "${INSTALL_PATH}")
    set(CORE_INSTALL_PATH "${INSTALL_PATH}/Core")
    set(PLUGIN_INSTALL_PATH "${INSTALL_PATH}/Plugin")
    set(DATA_INSTALL_PATH "${INSTALL_PATH}/Data")
    set(INSTALL_DESKTOP_FILE OFF)
else()
    set(RMG_INSTALL_PATH "bin")
    set(SYSTEM_LIB_INSTALL_PATH "lib")
    set(LIB_INSTALL_PATH "lib/RMG")
    set(CORE_INSTALL_PATH "lib/RMG/Core")
    set(PLUGIN_INSTALL_PATH "lib/RMG/Plugin")
    set(DATA_INSTALL_PATH "share/RMG")
    set(INSTALL_DESKTOP_FILE ON)
    set(APPDATA_INSTALL_PATH "share/metainfo/")
    set(DESKTOP_INSTALL_PATH "share/applications")
    set(ICON_INSTALL_PATH "share/icons/hicolor/scalable/apps/")
endif()

add_subdirectory(Source/3rdParty)
add_subdirectory(Source/3rdParty/7-Zip)
if (VRU)
    add_subdirectory(Source/3rdParty/vosk-api)
endif(VRU)

add_subdirectory(Source/RMG-Core)
add_subdirectory(Source/RMG)
add_subdirectory(Source/RMG-Audio)
add_subdirectory(Source/RMG-Input)
install(TARGETS RMG-Core
    DESTINATION ${SYSTEM_LIB_INSTALL_PATH}
)
install(TARGETS RMG
    DESTINATION ${RMG_INSTALL_PATH}
)
install(TARGETS RMG-Audio
    DESTINATION ${PLUGIN_INSTALL_PATH}/Audio
)
install(TARGETS RMG-Input
    DESTINATION ${PLUGIN_INSTALL_PATH}/Input
)

if (WIN32)
    add_subdirectory(Source/Installer)

    add_custom_target(bundle_dependencies
        COMMAND bash "${CMAKE_SOURCE_DIR}/Source/Script/BundleDependencies.sh" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/RMG.exe" "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}" "/mingw64/bin"
	)
endif(WIN32)

install(FILES ${MUPEN64PLUSCORE_LIB}
    DESTINATION ${CORE_INSTALL_PATH}
)
install(FILES ${MUPEN64PLUSCORE_INI} Data/font.ttf
    DESTINATION ${DATA_INSTALL_PATH}
)
if (VRU)
    install(FILES ${VOSK_API_LIBRARY}
        DESTINATION ${LIB_INSTALL_PATH}
    )
    install(FILES ${VOSK_API_MODEL}
        DESTINATION ${DATA_INSTALL_PATH}
    )
endif(VRU)
install(FILES ${SDL_GAMECONTROLLERDB}
    DESTINATION ${DATA_INSTALL_PATH}
)
file(GLOB GENERATED_CHEAT_FILES ${CMAKE_SOURCE_DIR}/Data/Cheats/Generated/*.cht)
file(GLOB CUSTOM_CHEAT_FILES ${CMAKE_SOURCE_DIR}/Data/Cheats/Custom/*.cht)
install(FILES ${GENERATED_CHEAT_FILES} ${CUSTOM_CHEAT_FILES}
    DESTINATION ${DATA_INSTALL_PATH}/Cheats
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP_CXD4}
    DESTINATION ${PLUGIN_INSTALL_PATH}/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP_HLE}
    DESTINATION ${PLUGIN_INSTALL_PATH}/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP_PARALLEL}
    DESTINATION ${PLUGIN_INSTALL_PATH}/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT_RAPHNET}
    DESTINATION ${PLUGIN_INSTALL_PATH}/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT_GCA}
    DESTINATION ${PLUGIN_INSTALL_PATH}/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_GFX_ANGRYLION}
    DESTINATION ${PLUGIN_INSTALL_PATH}/GFX
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
