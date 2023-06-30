#
# 7-Zip CMakeLists.txt
#
project(vosk-api)

set(VOSK_API_VERSION "0.3.45")

if (WIN32)
    set(VOSK_API_PLATFORM "win64")
    set(SO_EXT "dll")
else(WIN32)
    set(SO_EXT "so")
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(VOSK_API_PLATFORM "linux-aarch64")
    else()
        set(VOSK_API_PLATFORM "linux-x86_64")
    endif()
endif(WIN32)

set(VOSK_API_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/vosk-${VOSK_API_PLATFORM}-${VOSK_API_VERSION}/libvosk.${SO_EXT}
)

add_library(vosk-api IMPORTED SHARED ${VOSK_API_SOURCES})

set(VOSK_API_LIBRARY ${VOSK_API_SOURCES} PARENT_SCOPE)
set(VOSK_API_MODEL  "${CMAKE_CURRENT_SOURCE_DIR}/vosk-model-small-en-us-0.15.zip" PARENT_SCOPE)
