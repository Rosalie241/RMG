cmake_minimum_required(VERSION 3.9)

project(D3D12MemoryAllocator)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML-based API documentation (requires Doxygen)" OFF)
option(BUILD_SHARED_LIBS "Build D3D12 Memory Allocator shared library" OFF)

if(BUILD_DOCUMENTATION)
    if(DOXYGEN_FOUND)
        # set input and output files
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/src/Doxyfile)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        # request to configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        # note the option ALL which allows to build the docs together with the application
        add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    else()
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif()
endif()

# D3D12MemoryAllocator contains an sample application which is not build by default
option(D3D12MA_BUILD_SAMPLE "Build D3D12MemoryAllocator sample application" OFF)

message(STATUS "D3D12MA_BUILD_SAMPLE = ${D3D12MA_BUILD_SAMPLE}")

add_subdirectory(src)
