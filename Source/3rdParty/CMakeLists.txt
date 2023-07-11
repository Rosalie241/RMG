#
# 3rdParty CMakeLists.txt
#
include(ExternalProject)

set(THIRDPARTY_DIR ${CMAKE_BINARY_DIR}/Source/3rdParty)

find_program(RUST_CARGO cargo)
if(NO_RUST)
    set(BUILD_INPUT_GCA OFF)
elseif(${RUST_CARGO} STREQUAL "RUST_CARGO-NOTFOUND")
    message(WARNING "mupen64plus-input-gca will not be built due to not having cargo installed")
    set(BUILD_INPUT_GCA OFF)
else()
    set(BUILD_INPUT_GCA ON)
endif()

set(M64P_CORE_DIR ${THIRDPARTY_DIR}/mupen64plus-core)
if(WIN32)
    set(SO_EXT "dll")
    set(GCA_LIB "mupen64plus_input_gca.${SO_EXT}")
    set(CORE_FILE "mupen64plus.dll")
else(WIN32)
    set(SO_EXT "so")
    set(GCA_LIB "libmupen64plus_input_gca.${SO_EXT}")
    set(CORE_FILE "libmupen64plus.so")
endif(WIN32)

set(BASE_CMAKE_CONFIGURE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
if (USE_CCACHE)
    set(CMAKE_CONFIGURE_ARGS ${BASE_CMAKE_CONFIGURE_ARGS} -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache)
    set(MAKE_CC_COMPILER "ccache ${CMAKE_C_COMPILER}")
    set(MAKE_CXX_COMPILER "ccache ${CMAKE_CXX_COMPILER}")
else(USE_CCACHE)
    set(CMAKE_CONFIGURE_ARGS ${BASE_CMAKE_CONFIGURE_ARGS})
    set(MAKE_CC_COMPILER "${CMAKE_C_COMPILER}")
    set(MAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}")
endif(USE_CCACHE)

if(NOT NO_GIT_CLONE)
    set(MUPEN64PLUSCORE_URL           "https://github.com/Rosalie241/mupen64plus-core")
    set(MUPEN64PLUSCORE_TAG           "bb6949ad2ab268024ee4049279abc2f3c7fbdbb4")

    set(MUPEN64PLUS_RSP_CXD4_URL      "https://github.com/mupen64plus/mupen64plus-rsp-cxd4")
    set(MUPEN64PLUS_RSP_CXD4_TAG      "39f79201baa15890c4cbae92f2215a634cc3ee6d")

    set(MUPEN64PLUS_RSP_HLE_URL       "https://github.com/mupen64plus/mupen64plus-rsp-hle")
    set(MUPEN64PLUS_RSP_HLE_TAG       "88093cb43499eff53d343653cddcae2132af17ef")

    set(MUPEN64PLUS_RSP_PARALLEL_URL  "https://github.com/Rosalie241/parallel-rsp")
    set(MUPEN64PLUS_RSP_PARALLEL_TAG  "dbecbe3a120e428436c5972fa7b872cfc2c9db3e")

    set(MUPEN64PLUS_INPUT_RAPHNET_URL "https://github.com/raphnet/mupen64plus-input-raphnetraw")
    set(MUPEN64PLUS_INPUT_RAPHNET_TAG "86112413e98a8648edb11d199673cc24d5799af8")

    set(MUPEN64PLUS_INPUT_GCA_URL     "https://github.com/amatho/mupen64plus-input-gca")
    set(MUPEN64PLUS_INPUT_GCA_TAG     "21639fb13dfa797a7c0949ffd9bbda9a3456fc69")

    set(MUPEN64PLUS_GFX_ANGRYLION_URL "https://github.com/Rosalie241/angrylion-rdp-plus")
    set(MUPEN64PLUS_GFX_ANGRYLION_TAG "670abbc972bd430fa77291b0967dd73128314317")

    set(MUPEN64PLUS_GFX_GLIDEN64_URL  "https://github.com/Rosalie241/GLideN64")
    set(MUPEN64PLUS_GFX_GLIDEN64_TAG  "2f1b358028e700fcae2502a53f4f5e6822ce0367")

    set(MUPEN64PLUS_GFX_PARALLEL_URL  "https://github.com/Rosalie241/parallel-rdp-standalone")
    set(MUPEN64PLUS_GFX_PARALLEL_TAG  "2c2226517c4c8929e08ec944654867e26efe0cf5")

    set(DISCORD_RPC_URL               "https://github.com/discord/discord-rpc")
    set(DISCORD_RPC_TAG               "963aa9f3e5ce81a4682c6ca3d136cddda614db33")

    set(IMGUI_URL                     "https://github.com/ocornut/imgui")
    set(IMGUI_TAG                     "d7c8516a4b848c0291e3d75b627c0843f515f591")

    set(SDL_GAMECONTROLLERDB_URL      "https://github.com/gabomdq/SDL_GameControllerDB")
    set(SDL_GAMECONTROLLERDB_TAG      "5e25aeccb84fb166451443c4cd9fff349e8b0360")
endif(NOT NO_GIT_CLONE)

ExternalProject_Add(mupen64plus-core
    SOURCE_DIR mupen64plus-core/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY ${MUPEN64PLUSCORE_URL}
    GIT_TAG ${MUPEN64PLUSCORE_TAG}

    BUILD_COMMAND make all OSD=0 NEW_DYNAREC=1 KEYBINDINGS=0 ACCURATE_FPU=1 TARGET=${CORE_FILE} DEBUG=$<CONFIG:Debug> CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
    BUILD_IN_SOURCE False

    BINARY_DIR ${M64P_CORE_DIR}/projects/unix

    BUILD_BYPRODUCTS ${M64P_CORE_DIR}/projects/unix/${CORE_FILE}
)

set(APIDIR "${CMAKE_BINARY_DIR}/Source/3rdParty/mupen64plus-core/src/api")

ExternalProject_Add(mupen64plus-rsp-cxd4
    SOURCE_DIR mupen64plus-rsp-cxd4/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY ${MUPEN64PLUS_RSP_CXD4_URL}
    GIT_TAG ${MUPEN64PLUS_RSP_CXD4_TAG}

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug> POSTFIX= CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-rsp-cxd4/projects/unix

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-cxd4/projects/unix/mupen64plus-rsp-cxd4.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-rsp-hle
    SOURCE_DIR mupen64plus-rsp-hle/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY ${MUPEN64PLUS_RSP_HLE_URL}
    GIT_TAG ${MUPEN64PLUS_RSP_HLE_TAG}

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug> CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-rsp-hle/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-hle/projects/unix/mupen64plus-rsp-hle.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-rsp-parallel
    SOURCE_DIR mupen64plus-rsp-parallel

    GIT_REPOSITORY ${MUPEN64PLUS_RSP_PARALLEL_URL}
    GIT_TAG ${MUPEN64PLUS_RSP_PARALLEL_TAG}

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE True
    CMAKE_ARGS ${CMAKE_CONFIGURE_ARGS}

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-parallel/mupen64plus-rsp-parallel.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-input-raphnetraw
    SOURCE_DIR mupen64plus-input-raphnetraw/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY ${MUPEN64PLUS_INPUT_RAPHNET_URL}
    GIT_TAG ${MUPEN64PLUS_INPUT_RAPHNET_TAG}

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug> CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix/mupen64plus-input-raphnetraw.${SO_EXT}
    DEPENDS mupen64plus-core
)

if (BUILD_INPUT_GCA)
    ExternalProject_Add(mupen64plus-input-gca
        SOURCE_DIR mupen64plus-input-gca/

        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""

        GIT_REPOSITORY ${MUPEN64PLUS_INPUT_GCA_URL}
        GIT_TAG ${MUPEN64PLUS_INPUT_GCA_TAG}

        BUILD_COMMAND cargo build --release --features "m64p_compat"
        BUILD_IN_SOURCE True

        BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-gca/target/release/${GCA_LIB}
        DEPENDS mupen64plus-core
    )
endif(BUILD_INPUT_GCA)

set(GLIDEN64_DIR "${THIRDPARTY_DIR}/mupen64plus-video-GLideN64")
set(GLIDEN64_BUILD_DIR "${GLIDEN64_DIR}/projects/cmake")
set(GLIDEN64_CUSTOM_INI "${GLIDEN64_DIR}/ini/GLideN64.custom.ini")
if (WIN32)
	set(GLIDEN64_LIB "${GLIDEN64_BUILD_DIR}/mupen64plus-video-GLideN64.${SO_EXT}")
else(WIN32)
	set(GLIDEN64_LIB "${GLIDEN64_BUILD_DIR}/plugin/${CMAKE_BUILD_TYPE}/mupen64plus-video-GLideN64.${SO_EXT}")
endif(WIN32)

set(GLIDENUI_TRANSLATIONS_LANGS "de;es;fr;it;ja;pl;pt_BR")
foreach(LANG ${GLIDENUI_TRANSLATIONS_LANGS})
    list(APPEND GLIDENUI_TRANSLATIONS "${GLIDEN64_DIR}/translations/release/gliden64_${LANG}.qm")
endforeach()

ExternalProject_Add(mupen64plus-video-angrylion-plus
    SOURCE_DIR mupen64plus-video-angrylion-plus

    GIT_REPOSITORY ${MUPEN64PLUS_GFX_ANGRYLION_URL}
    GIT_TAG ${MUPEN64PLUS_GFX_ANGRYLION_TAG}

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE True
    CMAKE_ARGS -DBUILD_MUPEN64PLUS=ON -DBUILD_PROJECT64=OFF -DUSE_QT5=OFF ${CMAKE_CONFIGURE_ARGS}

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-video-angrylion-plus/mupen64plus-video-angrylion-plus.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-video-GLideN64
    SOURCE_DIR mupen64plus-video-GLideN64

    GIT_REPOSITORY ${MUPEN64PLUS_GFX_GLIDEN64_URL}
    GIT_TAG ${MUPEN64PLUS_GFX_GLIDEN64_TAG}

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    CMAKE_ARGS -DMUPENPLUSAPI=ON -DMUPENPLUSAPI_GLIDENUI=ON -DGLIDENUI_QT6=ON -DUSE_SYSTEM_LIBS=ON ${CMAKE_CONFIGURE_ARGS}

    SOURCE_SUBDIR ./src/
    BINARY_DIR ${GLIDEN64_BUILD_DIR}

    BUILD_BYPRODUCTS ${GLIDEN64_LIB}
)

ExternalProject_Add(mupen64plus-video-parallel
    SOURCE_DIR mupen64plus-video-parallel

    GIT_REPOSITORY ${MUPEN64PLUS_GFX_PARALLEL_URL}
    GIT_TAG ${MUPEN64PLUS_GFX_PARALLEL_TAG}

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE True
    CMAKE_ARGS -DUSE_QT5=OFF ${CMAKE_CONFIGURE_ARGS}

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-video-parallel/mupen64plus-video-parallel.${SO_EXT}
    DEPENDS mupen64plus-core
)

if (DISCORD_RPC)
    ExternalProject_Add(discord-rpc
        SOURCE_DIR discord-rpc

        GIT_REPOSITORY ${DISCORD_RPC_URL}
        GIT_TAG ${DISCORD_RPC_TAG}

        INSTALL_COMMAND ""

        BUILD_IN_SOURCE True
        CMAKE_ARGS -DBUILD_EXAMPLES=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON ${CMAKE_CONFIGURE_ARGS}

        BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/discord-rpc/src/libdiscord-rpc.a
    )
endif(DISCORD_RPC)

set(IMGUI_DIR ${THIRDPARTY_DIR}/imgui)
set(IMGUI_BUILD_DIR ${IMGUI_DIR}/build)
set(IMGUI_SOURCES_IN
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp 
    ${IMGUI_DIR}/imgui_widgets.cpp 
    ${IMGUI_DIR}/imgui.cpp 
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.h 
    ${IMGUI_DIR}/backends/imgui_impl_opengl3_loader.h 
)
set(IMGUI_SOURCES_OUT
    ${IMGUI_BUILD_DIR}/imgui_draw.cpp
    ${IMGUI_BUILD_DIR}/imgui_tables.cpp
    ${IMGUI_BUILD_DIR}/imgui_widgets.cpp
    ${IMGUI_BUILD_DIR}/imgui.cpp
    ${IMGUI_BUILD_DIR}/imgui_impl_opengl3.cpp
    ${IMGUI_BUILD_DIR}/imgui_impl_opengl3.h 
    ${IMGUI_BUILD_DIR}/imgui_impl_opengl3_loader.h 
)
ExternalProject_Add(imgui
    SOURCE_DIR imgui

    GIT_REPOSITORY ${IMGUI_URL}
    GIT_TAG ${IMGUI_TAG}

    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${IMGUI_BUILD_DIR}
    BUILD_COMMAND ${CMAKE_COMMAND} -E copy ${IMGUI_SOURCES_IN} ${IMGUI_BUILD_DIR}
    INSTALL_COMMAND ""

    BUILD_BYPRODUCTS ${IMGUI_SOURCES_OUT}
)

set(SDL_GAMECONTROLLERDB_DIR ${THIRDPARTY_DIR}/SDL_GameControllerDB)
set(SDL_GAMECONTROLLERDB_BUILD_DIR ${SDL_GAMECONTROLLERDB_DIR}/build)
set(SDL_GAMECONTROLLERDB_SOURCES_IN
    ${SDL_GAMECONTROLLERDB_DIR}/gamecontrollerdb.txt
)
set(SDL_GAMECONTROLLERDB_SOURCES_OUT
    ${SDL_GAMECONTROLLERDB_BUILD_DIR}/gamecontrollerdb.txt
)
ExternalProject_Add(SDL_GameControllerDB
    SOURCE_DIR SDL_GameControllerDB

    GIT_REPOSITORY ${SDL_GAMECONTROLLERDB_URL}
    GIT_TAG ${SDL_GAMECONTROLLERDB_TAG}

    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${SDL_GAMECONTROLLERDB_BUILD_DIR}
    BUILD_COMMAND ${CMAKE_COMMAND} -E copy ${SDL_GAMECONTROLLERDB_SOURCES_IN} ${SDL_GAMECONTROLLERDB_SOURCES_OUT}
    INSTALL_COMMAND ""

    BUILD_BYPRODUCTS ${SDL_GAMECONTROLLERDB_SOURCES_OUT}
)

ExternalProject_Get_property(mupen64plus-core BUILD_BYPRODUCTS)
set(MUPEN64PLUSCORE_LIB ${BUILD_BYPRODUCTS} PARENT_SCOPE)
set(MUPEN64PLUSCORE_INI ${M64P_CORE_DIR}/data/mupen64plus.ini PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-rsp-cxd4 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_RSP_CXD4 ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-rsp-hle BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_RSP_HLE ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-rsp-parallel BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_RSP_PARALLEL ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-input-raphnetraw BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_INPUT_RAPHNET ${BUILD_BYPRODUCTS} PARENT_SCOPE)

if (BUILD_INPUT_GCA)
    ExternalProject_Get_property(mupen64plus-input-gca BUILD_BYPRODUCTS)
    set(MUPEN64PLUS_PLUGIN_INPUT_GCA ${BUILD_BYPRODUCTS} PARENT_SCOPE)
endif(BUILD_INPUT_GCA)

ExternalProject_Get_property(mupen64plus-video-angrylion-plus BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_ANGRYLION ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-GLideN64 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_GLIDEN64 ${BUILD_BYPRODUCTS} ${GLIDENUI_TRANSLATIONS} ${GLIDEN64_CUSTOM_INI} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-parallel BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_PARALLEL ${BUILD_BYPRODUCTS} PARENT_SCOPE)

if (DISCORD_RPC)
    ExternalProject_Get_property(discord-rpc BUILD_BYPRODUCTS)
    set(DISCORD_RPC_LIB ${BUILD_BYPRODUCTS} PARENT_SCOPE)
endif(DISCORD_RPC)

ExternalProject_Get_property(imgui BUILD_BYPRODUCTS)
set(IMGUI_SOURCES ${BUILD_BYPRODUCTS} PARENT_SCOPE)
set(IMGUI_DIR ${THIRDPARTY_DIR}/imgui PARENT_SCOPE)

ExternalProject_Get_property(SDL_GameControllerDB BUILD_BYPRODUCTS)
set(SDL_GAMECONTROLLERDB ${BUILD_BYPRODUCTS} PARENT_SCOPE)
