cmake_minimum_required(VERSION 3.10)

project( GLideNHQ )

set(GLideNHQ_SOURCES
  TextureFilters.cpp
  TextureFilters_2xsai.cpp
  TextureFilters_hq2x.cpp
  TextureFilters_hq4x.cpp
  TextureFilters_xbrz.cpp
  TxCache.cpp
  TxDbg.cpp
  TxFilter.cpp
  TxFilterExport.cpp
  TxHiResLoader.cpp
  TxHiResCache.cpp
  TxHiResNoCache.cpp
  TxImage.cpp
  TxQuantize.cpp
  TxReSample.cpp
  TxTexCache.cpp
  TxUtil.cpp
)

if(MINGW OR PANDORA OR BCMHOST)
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../osal )
else(MINGW OR PANDORA OR BCMHOST)
  include_directories( inc ${CMAKE_CURRENT_SOURCE_DIR}/../osal )
endif(MINGW OR PANDORA OR BCMHOST)
LINK_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/lib )

if(UNIX)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(
      -DOS_MAC_OS_X
    )
    set(APPLE ON)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(
      -DNDEBUG
      -DOS_LINUX
    )
  endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(UNIX)

if(WIN32)
  LINK_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/lib )
  add_definitions(
	-DWIN32
	-DOS_WINDOWS
	-D_CRT_SECURE_NO_WARNINGS
  )
endif(WIN32)

# Build type

if( GLIDEN64_BUILD_TYPE STREQUAL "Debug")
	set( DEBUG_BUILD TRUE)
	add_definitions(
		-DDEBUG
	)
endif( GLIDEN64_BUILD_TYPE STREQUAL "Debug")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions( -D__MSC__)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(GCC_CPP11_COMPILE_FLAGS "-std=c++0x -Wno-unused-result")
    if ( NOT MINGW )
      SET ( PIC_FLAGS "-fPIC" )
    endif ( NOT MINGW)
    SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CPP11_COMPILE_FLAGS} -static ${PIC_FLAGS} " )
    SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -static ${PIC_FLAGS} " )
#SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CPP11_COMPILE_FLAGS} " )
endif()

add_definitions( -DTXFILTER_LIB)
if( GLIDEN64_BUILD_TYPE STREQUAL "Debug")
    SET( BUILD_VARIANT "debug" )
else()
    SET( BUILD_VARIANT "release" )
endif()
if( GLIDEN64_BUILD_TYPE STREQUAL "Debug")
  add_library( GLideNHQd STATIC ${GLideNHQ_SOURCES})
  set_target_properties(GLideNHQd PROPERTIES LINK_SEARCH_START_STATIC 1)
  set_target_properties(GLideNHQd PROPERTIES LINK_SEARCH_END_STATIC 1)
  if(MINGW OR BCMHOST OR APPLE OR USE_SYSTEM_LIBS)
    FIND_PACKAGE( ZLIB REQUIRED )
    FIND_PACKAGE( PNG REQUIRED )
    target_link_libraries(GLideNHQd
      ${PNG_LIBRARIES}
      ${ZLIB_LIBRARIES}
      osald
    )
  elseif(ANDROID)
      target_link_libraries(GLideNHQ PRIVATE
              ${ANDROID_LIB_PATH}/libs/${BUILD_VARIANT}/${ANDROID_ABI}/libpng.a
              z
              dl
              osal
              )
  else(MINGW OR BCMHOST OR APPLE OR USE_SYSTEM_LIBS)
    target_link_libraries(GLideNHQd PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng.a
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/libz.a
      dl
      osald
    )
  endif(MINGW OR BCMHOST OR APPLE OR USE_SYSTEM_LIBS)
else( GLIDEN64_BUILD_TYPE STREQUAL "Debug")
  add_library( GLideNHQ STATIC ${GLideNHQ_SOURCES})

#  set_target_properties(GLideNHQ PROPERTIES LINK_SEARCH_START_STATIC 1)
#  set_target_properties(GLideNHQ PROPERTIES LINK_SEARCH_END_STATIC 1)
#  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#  find_library(PNGLIB libpng.a)

  if(PANDORA)
    target_link_libraries(GLideNHQ PRIVATE
      /mnt/utmp/codeblocks/usr/lib/libpng.a
      /mnt/utmp/codeblocks/usr/lib/libz.a
      osal
    )
  elseif(BCMHOST OR MINGW OR APPLE OR USE_SYSTEM_LIBS)
    FIND_PACKAGE( ZLIB REQUIRED )
    FIND_PACKAGE( PNG REQUIRED )
    target_link_libraries(GLideNHQ
      ${PNG_LIBRARIES}
      ${ZLIB_LIBRARIES}
      osal
    )
  elseif(ANDROID)
    target_link_libraries(GLideNHQ PRIVATE
      ${ANDROID_LIB_PATH}/libs/${BUILD_VARIANT}/${ANDROID_ABI}/libpng.a
      z
      dl
      osal
    )
  else(PANDORA)
    target_link_libraries(GLideNHQ PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng.a
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/libz.a
      dl
      osal
    )
  endif(PANDORA)
endif( GLIDEN64_BUILD_TYPE STREQUAL "Debug")
