#
# Mupen64plus CMakeLists.txt
#
include(ExternalProject)

set(THIRDPARTY_DIR ${CMAKE_BINARY_DIR}/Source/3rdParty)

set(M64P_CORE_DIR ${THIRDPARTY_DIR}/mupen64plus-core)
if(WIN32)
    set(SO_EXT "dll")
    set(CORE_FILE "mupen64plus.dll")
    set(USE_NEW_DYNAREC "1")
else(WIN32)
    set(SO_EXT "so")
    set(CORE_FILE "libmupen64plus.so.2.0.0")
    # see https://github.com/mupen64plus/mupen64plus-core/issues/640
    set(USE_NEW_DYNAREC "0")
endif(WIN32)

ExternalProject_Add(mupen64plus-core
    SOURCE_DIR mupen64plus-core/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/Rosalie241/mupen64plus-core
    GIT_TAG 13729a892ee253e19627fe29887787817813dd9f

    BUILD_COMMAND make all OSD=0 NEW_DYNAREC=${USE_NEW_DYNAREC} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${M64P_CORE_DIR}/projects/unix
    
    BUILD_BYPRODUCTS ${M64P_CORE_DIR}/projects/unix/${CORE_FILE}
)

set(APIDIR "${CMAKE_BINARY_DIR}/Source/3rdParty/mupen64plus-core/src/api")

ExternalProject_Add(mupen64plus-audio-sdl
    SOURCE_DIR mupen64plus-audio-sdl2/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/Rosalie241/mupen64plus-audio-sdl2
    GIT_TAG cc6aca5a13b0b8ca7a2f9f5fa472ed3bb7f817db

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-audio-sdl2/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-audio-sdl2/projects/unix/mupen64plus-audio-sdl2.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-rsp-cxd4
    SOURCE_DIR mupen64plus-rsp-cxd4/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-rsp-cxd4
    GIT_TAG 094c6642d0571b683571a27f3118749d03f37b13

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug> SSE=SSE2
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-rsp-cxd4/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-cxd4/projects/unix/mupen64plus-rsp-cxd4-sse2.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-input-qt
    SOURCE_DIR mupen64plus-input-qt

    CMAKE_ARGS -DAPIDIR=${APIDIR} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/Rosalie241/mupen64plus-input-qt
    GIT_TAG e6cacb7fe6513cac4b30f7171d8ca7d1cd5fdb81

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE True
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-qt/libmupen64plus-input-qt.${SO_EXT}
    DEPENDS mupen64plus-core
)

set(GLIDEN64_DIR "${THIRDPARTY_DIR}/mupen64plus-video-GLideN64")
set(GLIDEN64_BUILD_DIR "${GLIDEN64_DIR}/projects/cmake")
if (MSYS)
	set(GLIDEN64_LIB "${GLIDEN64_BUILD_DIR}/mupen64plus-video-GLideN64.${SO_EXT}")
else()
	set(GLIDEN64_LIB "${GLIDEN64_BUILD_DIR}/plugin/${CMAKE_BUILD_TYPE}/mupen64plus-video-GLideN64.${SO_EXT}")
endif()

set(GLIDENUI_TRANSLATIONS_LANGS "de;es;fr;it;ja;pl;pt_BR")
foreach(LANG ${GLIDENUI_TRANSLATIONS_LANGS})
    list(APPEND GLIDENUI_TRANSLATIONS "${GLIDEN64_DIR}/translations/release/gliden64_${LANG}.qm")
endforeach()

ExternalProject_Add(mupen64plus-video-GLideN64
    SOURCE_DIR mupen64plus-video-GLideN64

    GIT_REPOSITORY https://github.com/gonetz/GLideN64
    GIT_TAG 3df9b2c3126d580b01b3d0412b5fecb9bf224678

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    CMAKE_ARGS -DMUPENPLUSAPI=1 -DMUPENPLUSAPI_GLIDENUI=1 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -Wno-dev

    SOURCE_SUBDIR ./src/
    BINARY_DIR ${GLIDEN64_BUILD_DIR}

    BUILD_BYPRODUCTS ${GLIDEN64_LIB}
    BUILD_BYPRODUCTS ${GLIDEN64_DIR}/ini/GLideN64.custom.ini
    BUILD_BYPRODUCTS ${GLIDENUI_TRANSLATIONS}
)

ExternalProject_Get_property(mupen64plus-core BUILD_BYPRODUCTS)
set(MUPEN64PLUSCORE_LIB ${BUILD_BYPRODUCTS} PARENT_SCOPE)
set(MUPEN64PLUSCORE_INI ${M64P_CORE_DIR}/data/mupen64plus.ini PARENT_SCOPE)
set(MUPEN64PLUSCORE_CHT ${M64P_CORE_DIR}/data/mupencheat.txt PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-audio-sdl BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_AUDIO ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-rsp-cxd4 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_RSP ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-input-qt BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_INPUT ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-GLideN64 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX ${BUILD_BYPRODUCTS} PARENT_SCOPE)
