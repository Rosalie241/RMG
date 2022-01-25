#
# 3rdParty CMakeLists.txt
#
include(ExternalProject)

set(THIRDPARTY_DIR ${CMAKE_BINARY_DIR}/Source/3rdParty)

find_program(RUST_CARGO cargo)
if(${RUST_CARGO} STREQUAL "RUST_CARGO-NOTFOUND")
    message(WARNING "mupen64plus-input-gca will not be built due to not having cargo installed")
    set(BUILD_INPUT_GCA "0")
else()
    # disabled because build failure
    set(BUILD_INPUT_GCA "0")
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

if(NOT NO_GIT_CLONE)
    set(MUPEN64PLUSCORE_URL           "https://github.com/Rosalie241/mupen64plus-core")
    set(MUPEN64PLUSCORE_TAG           "6777f942eca0e94b1d433651b881e1bc18e903e6")
    
    set(MUPEN64PLUS_RSP_HLE_URL       "https://github.com/mupen64plus/mupen64plus-rsp-hle")
    set(MUPEN64PLUS_RSP_HLE_TAG       "88093cb43499eff53d343653cddcae2132af17ef")
    
    set(MUPEN64PLUS_RSP_PARALLEL_URL  "https://github.com/loganmc10/parallel-rsp")
    set(MUPEN64PLUS_RSP_PARALLEL_TAG  "a9646b27c2ef399e5917735b4d84ebe93edcce39")
    
    set(MUPEN64PLUS_INPUT_QT_URL      "https://github.com/Rosalie241/mupen64plus-input-qt")
    set(MUPEN64PLUS_INPUT_QT_TAG      "e6cacb7fe6513cac4b30f7171d8ca7d1cd5fdb81")
    
    set(MUPEN64PLUS_INPUT_RAPHNET_URL "https://github.com/raphnet/mupen64plus-input-raphnetraw")
    set(MUPEN64PLUS_INPUT_RAPHNET_TAG "86112413e98a8648edb11d199673cc24d5799af8")

    set(MUPEN64PLUS_INPUT_GCA_URL     "https://github.com/amatho/mupen64plus-input-gca")
    set(MUPEN64PLUS_INPUT_GCA_TAG     "d440611d3639fa974c7d52bd0ed2079d16428d5e")

    set(MUPEN64PLUS_GFX_GLIDEN64_URL  "https://github.com/gonetz/GLideN64")
    set(MUPEN64PLUS_GFX_GLIDEN64_TAG  "8343fd05c9a01ce2d73f7e32bb8355036759a05b")

    set(MUPEN64PLUS_GFX_PARALLEL_URL  "https://github.com/loganmc10/parallel-rdp-standalone")
    set(MUPEN64PLUS_GFX_PARALLEL_TAG  "2efeaf014bad03ec26e081c22d8ca3566003b1ea")
endif(NOT NO_GIT_CLONE)

ExternalProject_Add(mupen64plus-core
    SOURCE_DIR mupen64plus-core/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY ${MUPEN64PLUSCORE_URL}
    GIT_TAG ${MUPEN64PLUSCORE_TAG}

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

    GIT_REPOSITORY ${MUPEN64PLUS_RSP_HLE_URL}
    GIT_TAG ${MUPEN64PLUS_RSP_HLE_TAG}

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
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
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-rsp-parallel/mupen64plus-rsp-parallel.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-input-qt
    SOURCE_DIR mupen64plus-input-qt

    CMAKE_ARGS -DAPIDIR=${APIDIR} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    INSTALL_COMMAND ""

    GIT_REPOSITORY ${MUPEN64PLUS_INPUT_QT_URL}
    GIT_TAG ${MUPEN64PLUS_INPUT_QT_TAG}

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE True
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-qt/libmupen64plus-input-qt.${SO_EXT}
    DEPENDS mupen64plus-core
)

ExternalProject_Add(mupen64plus-input-raphnetraw
    SOURCE_DIR mupen64plus-input-raphnetraw/

    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""

    GIT_REPOSITORY ${MUPEN64PLUS_INPUT_RAPHNET_URL}
    GIT_TAG ${MUPEN64PLUS_INPUT_RAPHNET_TAG}

    BUILD_COMMAND make all APIDIR=${APIDIR} DEBUG=$<CONFIG:Debug>
    BUILD_IN_SOURCE False

    BINARY_DIR ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix
    
    BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-raphnetraw/projects/unix/mupen64plus-input-raphnetraw.${SO_EXT}
    DEPENDS mupen64plus-core
)

if (${BUILD_INPUT_GCA} STREQUAL "1")
    ExternalProject_Add(mupen64plus-input-gca
        SOURCE_DIR mupen64plus-input-gca/
    
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""
    
        GIT_REPOSITORY ${MUPEN64PLUS_INPUT_GCA_URL}
        GIT_TAG ${MUPEN64PLUS_INPUT_GCA_TAG}
    
        BUILD_COMMAND cargo build --release
        BUILD_IN_SOURCE True
    
        BUILD_BYPRODUCTS ${THIRDPARTY_DIR}/mupen64plus-input-gca/target/release/${GCA_LIB}
        DEPENDS mupen64plus-core
    )
endif()

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

    GIT_REPOSITORY ${MUPEN64PLUS_GFX_GLIDEN64_URL}
    GIT_TAG ${MUPEN64PLUS_GFX_GLIDEN64_TAG}

    INSTALL_COMMAND ""

    BUILD_IN_SOURCE False
    CMAKE_ARGS -DMUPENPLUSAPI=1 -DMUPENPLUSAPI_GLIDENUI=1 -DUSE_SYSTEM_LIBS=ON -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

    SOURCE_SUBDIR ./src/
    BINARY_DIR ${GLIDEN64_BUILD_DIR}

    BUILD_BYPRODUCTS ${GLIDEN64_LIB}
    BUILD_BYPRODUCTS ${GLIDEN64_DIR}/ini/GLideN64.custom.ini
    BUILD_BYPRODUCTS ${GLIDENUI_TRANSLATIONS}
)

ExternalProject_Add(mupen64plus-video-parallel
    SOURCE_DIR mupen64plus-video-parallel

    GIT_REPOSITORY ${MUPEN64PLUS_GFX_PARALLEL_URL}
    GIT_TAG ${MUPEN64PLUS_GFX_PARALLEL_TAG}

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

if (${BUILD_INPUT_GCA} STREQUAL "1")
    ExternalProject_Get_property(mupen64plus-input-gca BUILD_BYPRODUCTS)
    set(MUPEN64PLUS_PLUGIN_INPUT_GCA ${BUILD_BYPRODUCTS} PARENT_SCOPE)
endif()

ExternalProject_Get_property(mupen64plus-video-GLideN64 BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_GLIDEN64 ${BUILD_BYPRODUCTS} PARENT_SCOPE)

ExternalProject_Get_property(mupen64plus-video-parallel BUILD_BYPRODUCTS)
set(MUPEN64PLUS_PLUGIN_GFX_PARALLEL ${BUILD_BYPRODUCTS} PARENT_SCOPE)
