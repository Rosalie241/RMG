#
# Mupen64plus CMakeLists.txt
#
include(ExternalProject)

set(M64P_CORE_DIR ${CMAKE_BINARY_DIR}/External/mupen64plus-core)
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

    BUILD_COMMAND make all OSD=0 NO_ASM=1
    BUILD_IN_SOURCE False

    BINARY_DIR ${M64P_CORE_DIR}/projects/unix
    
    BUILD_BYPRODUCTS ${M64P_CORE_DIR}/projects/unix/${CORE_FILE}
)

set(APIDIR "${CMAKE_BINARY_DIR}/External/mupen64plus-core/src/api")

ExternalProject_Add(mupen64plus-audio-sdl
    SOURCE_DIR mupen64plus-audio-sdl/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-audio-sdl
    GIT_TAG 39b8cb778c4690de98c8b9497578662cece356cc

    BUILD_COMMAND make all APIDIR=${APIDIR}
    BUILD_IN_SOURCE False

    BINARY_DIR ${CMAKE_BINARY_DIR}/External/mupen64plus-audio-sdl/projects/unix
    
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/External/mupen64plus-audio-sdl/projects/unix/mupen64plus-audio-sdl.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-rsp-hle
    SOURCE_DIR mupen64plus-rsp-hle/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-rsp-hle
    GIT_TAG 72a1deca7d57f688cf20569790077bf7ab2cbe7c

    BUILD_COMMAND make all APIDIR=${APIDIR}
    BUILD_IN_SOURCE False

    BINARY_DIR ${CMAKE_BINARY_DIR}/External/mupen64plus-rsp-hle/projects/unix
    
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/External/mupen64plus-rsp-hle/projects/unix/mupen64plus-rsp-hle.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-input-raphnetraw
    SOURCE_DIR mupen64plus-input-raphnetraw

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/raphnet/mupen64plus-input-raphnetraw
    GIT_TAG 741f37cd9dae981235e57ebdc3083be5f5755439

    BUILD_COMMAND make all APIDIR=${APIDIR}
    BUILD_IN_SOURCE False

    BINARY_DIR ${CMAKE_BINARY_DIR}/External/mupen64plus-input-raphnetraw/projects/unix
    
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/External/mupen64plus-input-raphnetraw/projects/unix/mupen64plus-input-raphnetraw.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-video-GLideN64
    SOURCE_DIR mupen64plus-video-GLideN64

    GIT_REPOSITORY https://github.com/Rosalie241/GLideN64
    GIT_TAG 2021a8e674dbcd744fd7fddeb1eff9e3beb9deb8

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE True
    CMAKE_ARGS -DMUPENPLUSAPI=1 -DMUPENPLUSAPI_GLIDENUI=1
    SOURCE_SUBDIR ./src

    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/External/mupen64plus-video-GLideN64/src/plugin/Release/mupen64plus-video-GLideN64.${SO_EXT}
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
