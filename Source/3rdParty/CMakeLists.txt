#
# 3rdParty CMakeLists.txt
#
include(ExternalProject)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(M64P_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-core)
if(WIN32)
    set(SO_EXT "dll")
    set(GCA_LIB "mupen64plus_input_gca.${SO_EXT}")
    set(CORE_FILE "mupen64plus.dll")
else(WIN32)
    set(SO_EXT "so")
    set(GCA_LIB "libmupen64plus_input_gca.${SO_EXT}")
    set(CORE_FILE "libmupen64plus.so")
endif(WIN32)

set(BASE_CMAKE_CONFIGURE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS})
if (USE_CCACHE)
    set(CMAKE_CONFIGURE_ARGS ${BASE_CMAKE_CONFIGURE_ARGS} -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache)
    set(MAKE_CC_COMPILER "ccache ${CMAKE_C_COMPILER}")
    set(MAKE_CXX_COMPILER "ccache ${CMAKE_CXX_COMPILER}")
else(USE_CCACHE)
    set(CMAKE_CONFIGURE_ARGS ${BASE_CMAKE_CONFIGURE_ARGS})
    set(MAKE_CC_COMPILER "${CMAKE_C_COMPILER}")
    set(MAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}")
endif(USE_CCACHE)

# use sub-make when available
if ("${CMAKE_GENERATOR}" STREQUAL "Unix Makefiles" OR
    "${CMAKE_GENERATOR}" STREQUAL "MSYS Makefiles")
    set(MAKE_CMD "$(MAKE)")
else()
    set(MAKE_CMD "make")
endif()

if ("${CMAKE_C_FLAGS}" STREQUAL "")
    if (USE_LTO)
        set(MAKE_OPTFLAGS "-O3 -flto")
    else(USE_LTO)
        set(MAKE_OPTFLAGS "-O3")
    endif(USE_LTO)
else()
    set(MAKE_OPTFLAGS "${CMAKE_C_FLAGS}")
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo" OR
    "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(MAKE_DEBUG "1")
else()
    set(MAKE_DEBUG "0")
endif()

# TODO: when SDL3_net has made a release,
# remove the vendored SDL3_net
set(SDL3NET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL_net)
set(SDL3NET_SOURCES
    ${SDL3NET_DIR}/src/SDL_net.c
)
add_library(SDL3_net STATIC ${SDL3NET_SOURCES})
target_include_directories(SDL3_net PRIVATE ${SDL3NET_DIR}/include)
set(SDL3NET_CFLAGS "-DUSE_SDL3NET -I${SDL3NET_DIR}/include")
set(SDL3NET_LDLIBS "$<TARGET_FILE:SDL3_net>")
if (WIN32)
    set(SDL3NET_LDLIBS "${SDL3NET_LDLIBS} -liphlpapi -lws2_32")
endif (WIN32)

ExternalProject_Add(mupen64plus-core
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-core/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    BUILD_COMMAND ${MAKE_CMD} all -f ${M64P_CORE_DIR}/projects/unix/Makefile 
        SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-core/src 
        SUBDIR=${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-core/subprojects 
        OSD=0 NEW_DYNAREC=1 NO_ASM=$<BOOL:${NO_ASM}> 
        KEYBINDINGS=0 ACCURATE_FPU=1 VULKAN=0 NETPLAY=$<BOOL:${NETPLAY}> 
        TARGET=${CORE_FILE} DEBUG=${MAKE_DEBUG}
        CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
        OPTFLAGS=${MAKE_OPTFLAGS}
        # TODO: when SDL3_net has made a release,
        # remove the vendored SDL3_net
        SDLNET_CFLAGS=${SDL3NET_CFLAGS} SDLNET_LDLIBS=${SDL3NET_LDLIBS}
    BUILD_IN_SOURCE False
    BUILD_ALWAYS True
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-core

    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-core/${CORE_FILE}
    DEPENDS SDL3_net
)

set(APIDIR "${CMAKE_SOURCE_DIR}/Source/3rdParty/mupen64plus-core/src/api")

ExternalProject_Add(mupen64plus-rsp-cxd4
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-rsp-cxd4/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    BUILD_COMMAND ${MAKE_CMD} all -f ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-rsp-cxd4/projects/unix/Makefile
        SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-rsp-cxd4
        APIDIR=${APIDIR} DEBUG=${MAKE_DEBUG} POSTFIX= 
        CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
        OPTFLAGS=${MAKE_OPTFLAGS}
    BUILD_IN_SOURCE False
    BUILD_ALWAYS True
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-rsp-cxd4

    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-rsp-cxd4/mupen64plus-rsp-cxd4.${SO_EXT}
)

ExternalProject_Add(mupen64plus-rsp-hle
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-rsp-hle/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    BUILD_COMMAND ${MAKE_CMD} all -f ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-rsp-hle/projects/unix/Makefile
        SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-rsp-hle/src
        APIDIR=${APIDIR} DEBUG=${MAKE_DEBUG}
        CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
        OPTFLAGS=${MAKE_OPTFLAGS}
    BUILD_IN_SOURCE False
    BUILD_ALWAYS True
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-rsp-hle

    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-rsp-hle/mupen64plus-rsp-hle.${SO_EXT}
)

ExternalProject_Add(mupen64plus-rsp-parallel
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-rsp-parallel/

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    BUILD_ALWAYS True
    CMAKE_ARGS ${CMAKE_CONFIGURE_ARGS}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-rsp-parallel

    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-rsp-parallel/mupen64plus-rsp-parallel.${SO_EXT}
)

ExternalProject_Add(mupen64plus-input-raphnetraw
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-input-raphnetraw/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    BUILD_COMMAND ${MAKE_CMD} all -f ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-input-raphnetraw/projects/unix/Makefile
        SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-input-raphnetraw/src
        APIDIR=${APIDIR} DEBUG=${MAKE_DEBUG}
        CC=${MAKE_CC_COMPILER} CXX=${MAKE_CXX_COMPILER}
        OPTFLAGS=${MAKE_OPTFLAGS}
    BUILD_IN_SOURCE False
    BUILD_ALWAYS True
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-input-raphnetraw

    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-input-raphnetraw/mupen64plus-input-raphnetraw.${SO_EXT}
)

set(GLIDEN64_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-video-GLideN64")
set(GLIDEN64_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-video-GLideN64")
set(GLIDEN64_CUSTOM_INI "${GLIDEN64_DIR}/ini/GLideN64.custom.ini")
# taken from GLideN64's CMakeLists.txt
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR
    "${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel" OR
    "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set(GLIDEN64_BUILD_TYPE Release)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(GLIDEN64_BUILD_TYPE Debug)
endif()

set(GLIDENUI_TRANSLATIONS_LANGS "de;es;fr;it;ja;pl;pt_BR")
foreach(LANG ${GLIDENUI_TRANSLATIONS_LANGS})
    list(APPEND GLIDENUI_TRANSLATIONS "${GLIDEN64_DIR}/translations/release/gliden64_${LANG}.qm")
endforeach()

if (USE_ANGRYLION)
    ExternalProject_Add(mupen64plus-video-angrylion-plus
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-video-angrylion-plus

        INSTALL_COMMAND ""

        BUILD_IN_SOURCE False
        BUILD_ALWAYS True
        CMAKE_ARGS -DBUILD_MUPEN64PLUS=ON -DBUILD_PROJECT64=OFF -DUSE_QT5=OFF ${CMAKE_CONFIGURE_ARGS}
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-video-angrylion-plus

        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-video-angrylion-plus/mupen64plus-video-angrylion-plus.${SO_EXT}
    )
endif(USE_ANGRYLION)

ExternalProject_Add(mupen64plus-video-GLideN64
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-video-GLideN64

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    BUILD_ALWAYS True
    CMAKE_ARGS -DMUPENPLUSAPI=ON -DMUPENPLUSAPI_GLIDENUI=ON -DGLIDENUI_QT6=ON -DUSE_SYSTEM_LIBS=ON ${CMAKE_CONFIGURE_ARGS}

    SOURCE_SUBDIR ./src/
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-video-GLideN64

    BUILD_BYPRODUCTS ${GLIDEN64_BUILD_DIR}/plugin/${GLIDEN64_BUILD_TYPE}/mupen64plus-video-GLideN64.${SO_EXT}
)

ExternalProject_Add(mupen64plus-video-parallel
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mupen64plus-video-parallel

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    BUILD_ALWAYS True
    CMAKE_ARGS -DCONFIG_GUI=ON ${CMAKE_CONFIGURE_ARGS}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-video-parallel

    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/mupen64plus-video-parallel/mupen64plus-video-parallel.${SO_EXT}
)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp 
    ${IMGUI_DIR}/imgui_widgets.cpp 
    ${IMGUI_DIR}/imgui.cpp 
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.h 
    ${IMGUI_DIR}/backends/imgui_impl_opengl3_loader.h 
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PRIVATE ${IMGUI_DIR})

set(SDL_GAMECONTROLLERDB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL_GameControllerDB)
set(SDL_GAMECONTROLLERDB_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/SDL_GameControllerDB)
set(SDL_GAMECONTROLLERDB_SOURCES_IN
    ${SDL_GAMECONTROLLERDB_DIR}/gamecontrollerdb.txt
)
set(SDL_GAMECONTROLLERDB_SOURCES_OUT
    ${SDL_GAMECONTROLLERDB_BUILD_DIR}/gamecontrollerdb.txt
)
ExternalProject_Add(SDL_GameControllerDB
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL_GameControllerDB

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

if (USE_ANGRYLION)
    ExternalProject_Get_property(mupen64plus-video-angrylion-plus BUILD_BYPRODUCTS)
    set(MUPEN64PLUS_PLUGIN_GFX_ANGRYLION ${BUILD_BYPRODUCTS} PARENT_SCOPE)
endif(USE_ANGRYLION)

ExternalProject_Get_property(mupen64plus-video-GLideN64 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_GLIDEN64 ${BUILD_BYPRODUCTS} PARENT_SCOPE)
set(MUPEN64PLUS_PLUGIN_GFX_GLIDEN64_DATA ${GLIDENUI_TRANSLATIONS} ${GLIDEN64_CUSTOM_INI} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-parallel BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_PARALLEL ${BUILD_BYPRODUCTS} PARENT_SCOPE)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui PARENT_SCOPE)

ExternalProject_Get_property(SDL_GameControllerDB BUILD_BYPRODUCTS)
set(SDL_GAMECONTROLLERDB ${BUILD_BYPRODUCTS} PARENT_SCOPE)
