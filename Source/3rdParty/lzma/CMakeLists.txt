#
# lzma CMakeLists.txt
#
project(lzma)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(lzma_SOURCES
    Bcj2.c
    Bra.c
    Bra86.c
    BraIA64.c
    CpuArch.c
    Delta.c
    Lzma2Dec.c
    LzmaDec.c
    Ppmd7.c
    Ppmd7Dec.c
    7zCrc.c
    7zCrcOpt.c
    Sha256.c
    Sha256Opt.c
    7zAlloc.c
    7zArcIn.c
    7zBuf.c
    7zBuf2.c
    7zDec.c
    7zFile.c
    7zStream.c
)

add_definitions(-D_7ZIP_PPMD_SUPPPORT -DNDEBUG -D_REENTRANT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
add_library(lzma STATIC ${lzma_SOURCES})
