#
# 3rdParty CMakeLists.txt
#
include(ExternalProject)

set(THIRDPARTY_DIR ${CMAKE_BINARY_DIR}/Source/3rdParty)

set(M64P_CORE_DIR ${THIRDPARTY_DIR}/mupen64plus-core)
if(WIN32)
    set(SO_EXT "dll")
    set(CORE_FILE "mupen64plus.dll")
else(WIN32)
    set(SO_EXT "so")
    set(CORE_FILE "libmupen64plus.so")
endif(WIN32)

ExternalProject_Add(mupen64plus-core
    SOURCE_DIR mupen64plus-core/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-core
    GIT_TAG 57828d930280554c7400bb2bcf1e46c7f2ee8373

    BUILD_COMMAND make all OSD=0 NEW_DYNAREC=1 TARGET=${CORE_FILE} DEBUG=$<CONFIG:Debug> KEYBINDINGS=0
    BUILD_IN_SOURCE False

    BINARY_DIR ${M64P_CORE_DIR}/projects/unix
    
    BUILD_BYPRODUCTS ${M64P_CORE_DIR}/projects/unix/${CORE_FILE}
)

set(APIDIR "${CMAKE_BINARY_DIR}/Source/3rdParty/mupen64plus-core/src/api")

ExternalProject_Add(mupen64plus-rsp-hle
    SOURCE_DIR mupen64plus-rsp-hle/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/mupen64plus/mupen64plus-rsp-hle
    GIT_TAG 88093cb43499eff53d343653cddcae2132af17ef

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-rsp-hle/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-hle/projects/unix/mupen64plus-rsp-hle.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-rsp-parallel
    SOURCE_DIR mupen64plus-rsp-parallel

    GIT_REPOSITORY https://github.com/loganmc10/parallel-rsp
    GIT_TAG a9646b27c2ef399e5917735b4d84ebe93edcce39

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE True
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-parallel/mupen64plus-rsp-parallel.${SO_EXT}
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

ExternalProject_Add(mupen64plus-input-raphnetraw
    SOURCE_DIR mupen64plus-input-raphnetraw/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY https://github.com/raphnet/mupen64plus-input-raphnetraw
    GIT_TAG 86112413e98a8648edb11d199673cc24d5799af8

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix/mupen64plus-input-raphnetraw.${SO_EXT}
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
    GIT_TAG 8343fd05c9a01ce2d73f7e32bb8355036759a05b

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    CMAKE_ARGS -DMUPENPLUSAPI=1 -DMUPENPLUSAPI_GLIDENUI=1 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -Wno-dev

    SOURCE_SUBDIR ./src/
    BINARY_DIR ${GLIDEN64_BUILD_DIR}

    BUILD_BYPRODUCTS ${GLIDEN64_LIB}
    BUILD_BYPRODUCTS ${GLIDEN64_DIR}/ini/GLideN64.custom.ini
    BUILD_BYPRODUCTS ${GLIDENUI_TRANSLATIONS}
)

ExternalProject_Add(mupen64plus-video-parallel
    SOURCE_DIR mupen64plus-video-parallel

    GIT_REPOSITORY https://github.com/loganmc10/parallel-rdp-standalone
    GIT_TAG 2efeaf014bad03ec26e081c22d8ca3566003b1ea

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE True
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-video-parallel/mupen64plus-video-parallel.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Get_property(mupen64plus-core BUILD_BYPRODUCTS)
set(MUPEN64PLUSCORE_LIB ${BUILD_BYPRODUCTS} PARENT_SCOPE)
set(MUPEN64PLUSCORE_INI ${M64P_CORE_DIR}/data/mupen64plus.ini PARENT_SCOPE)
set(MUPEN64PLUSCORE_CHT ${M64P_CORE_DIR}/data/mupencheat.txt PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-rsp-hle BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_RSP_HLE ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-rsp-parallel BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_RSP_PARALLEL ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-input-qt BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_INPUT_QT ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-input-raphnetraw BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_INPUT_RAPHNET ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-GLideN64 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_GLIDEN64 ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-parallel BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_PARALLEL ${BUILD_BYPRODUCTS} PARENT_SCOPE)
