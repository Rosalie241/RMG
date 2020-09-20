#
# Mupen64plus CMakeLists.txt
#
include(ExternalProject)

set(THIRDPARTY_DIR ${CMAKE_BINARY_DIR}/Source/3rdParty)

set(M64P_CORE_DIR ${THIRDPARTY_DIR}/mupen64plus-core)
if(WIN32)
    set(SO_EXT "dll")
    set(CORE_FILE "mupen64plus.dll")
else(WIN32)
    set(SO_EXT "so")
    set(CORE_FILE "libmupen64plus.so.2.0.0")
endif(WIN32)

ExternalProject_Add(mupen64plus-core
    SOURCE_DIR mupen64plus-core/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-core
    GIT_TAG 59a501ec8915f3c6f20528b076c8dcbe95bb3830

    BUILD_COMMAND make all OSD=0 NO_ASM=1 DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${M64P_CORE_DIR}/projects/unix
    
    BUILD_BYPRODUCTS ${M64P_CORE_DIR}/projects/unix/${CORE_FILE}
)

set(APIDIR "${CMAKE_BINARY_DIR}/Source/3rdParty/mupen64plus-core/src/api")

ExternalProject_Add(mupen64plus-audio-sdl
    SOURCE_DIR mupen64plus-audio-sdl/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-audio-sdl
    GIT_TAG 39b8cb778c4690de98c8b9497578662cece356cc

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-audio-sdl/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-audio-sdl/projects/unix/mupen64plus-audio-sdl.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-rsp-hle
    SOURCE_DIR mupen64plus-rsp-hle/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-rsp-hle
    GIT_TAG 72a1deca7d57f688cf20569790077bf7ab2cbe7c

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-rsp-hle/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-hle/projects/unix/mupen64plus-rsp-hle.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-input-raphnetraw
    SOURCE_DIR mupen64plus-input-raphnetraw

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/raphnet/mupen64plus-input-raphnetraw
    GIT_TAG 741f37cd9dae981235e57ebdc3083be5f5755439

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix/mupen64plus-input-raphnetraw.${SO_EXT}
    DEPENDS mupen64plus-core
)

set(GLIDEN64_BUILD_DIR "${THIRDPARTY_DIR}/mupen64plus-video-GLideN64/projects/cmake")
if (MSYS)
	set(GLIDEN64_LIB "${GLIDEN64_BUILD_DIR}/mupen64plus-video-GLideN64.${SO_EXT}")
else()
	set(GLIDEN64_LIB "${GLIDEN64_BUILD_DIR}/plugin/${CMAKE_BUILD_TYPE}/mupen64plus-video-GLideN64.${SO_EXT}")
endif()

ExternalProject_Add(mupen64plus-video-GLideN64
    SOURCE_DIR mupen64plus-video-GLideN64

    GIT_REPOSITORY https://github.com/gonetz/GLideN64
    GIT_TAG 1f5f9874d947bf5d10b9ef0e4f80cc6b4fa10d0d

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    CMAKE_ARGS -DMUPENPLUSAPI=1 -DMUPENPLUSAPI_GLIDENUI=1 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -Wno-dev

    SOURCE_SUBDIR ./src/
    BINARY_DIR ${GLIDEN64_BUILD_DIR}

    BUILD_BYPRODUCTS ${GLIDEN64_LIB}
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-video-GLideN64/ini/GLideN64.custom.ini
)

ExternalProject_Get_property(mupen64plus-core BUILD_BYPRODUCTS)
set(MUPEN64PLUSCORE_LIB ${BUILD_BYPRODUCTS} PARENT_SCOPE)
set(MUPEN64PLUSCORE_INI ${M64P_CORE_DIR}/data/mupen64plus.ini PARENT_SCOPE)
set(MUPEN64PLUSCORE_CHT ${M64P_CORE_DIR}/data/mupencheat.txt PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-audio-sdl BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_AUDIO ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-rsp-hle BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_RSP ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-input-raphnetraw BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_INPUT ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-GLideN64 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX ${BUILD_BYPRODUCTS} PARENT_SCOPE)
