cmake_minimum_required(VERSION 3.16...4.0)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(cmake_build_type_undefined 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# See docs/release_checklist.md
set(MAJOR_VERSION 3)
set(MINOR_VERSION 0)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION 3.0.0)

project(SDL3_net
    LANGUAGES C
    VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
)

include("${SDL3_net_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake")
include("${SDL3_net_SOURCE_DIR}/cmake/PrivateSdlFunctions.cmake")
include("${SDL3_net_SOURCE_DIR}/cmake/sdlmanpages.cmake")
sdl_calculate_derived_version_variables(${MAJOR_VERSION} ${MINOR_VERSION} ${MICRO_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SDLNET_ROOTPROJECT ON)
else()
    set(SDLNET_ROOTPROJECT OFF)
endif()

# By default, configure in RelWithDebInfo configuration
if(SDLNET_ROOTPROJECT)
    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        # The first item in CMAKE_CONFIGURATION_TYPES is the default configuration
        if(DEFINED CMAKE_CONFIGURATION_TYPES AND "RelWithDebInfo" IN_LIST CMAKE_CONFIGURATION_TYPES)
            list(REMOVE_ITEM CMAKE_CONFIGURATION_TYPES "RelWithDebInfo")
            list(INSERT CMAKE_CONFIGURATION_TYPES 0 "RelWithDebInfo")
            set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "CMake configuration types" FORCE)
        endif()
    else()
        if(cmake_build_type_undefined)
            set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "CMake build type" FORCE)
        endif()
    endif()
endif()

set(SDLNET_SAMPLES_DEFAULT ${SDLNET_ROOTPROJECT})
if(ANDROID)
    set(SDLNET_SAMPLES_DEFAULT OFF)
endif()

set(sdl3net_install_enableable ON)
if((TARGET SDL3-shared OR TARGET SDL3-static) AND SDL_DISABLE_INSTALL)
    # Cannot install SDL3_net when SDL3 is built in same built, and is not installed.
    set(sdl3net_install_enableable OFF)
endif()

if(NOT DEFINED CMAKE_FIND_PACKAGE_PREFER_CONFIG)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
endif()

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(PLATFORM_SUPPORTS_SHARED ON)
if(EMSCRIPTEN OR VITA OR PSP OR PS2 OR N3DS OR RISCOS)
    set(PLATFORM_SUPPORTS_SHARED OFF)
endif()

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ${PLATFORM_SUPPORTS_SHARED})
cmake_dependent_option(BUILD_SHARED_LIBS "Build the library as a shared library" ON PLATFORM_SUPPORTS_SHARED OFF)

cmake_dependent_option(SDLNET_INSTALL "Enable SDL3_net install target" ${SDLNET_ROOTPROJECT} "${sdl3net_install_enableable}" OFF)
cmake_dependent_option(SDLNET_INSTALL_MAN "Install man pages for SDL3_net" OFF "SDLNET_INSTALL" OFF)
cmake_dependent_option(SDLNET_RELOCATABLE "Create relocatable SDL_net package" "${MSVC}" SDLNET_INSTALL OFF)
option(SDLNET_WERROR "Treat warnings as errors" OFF)

option(SDLNET_SAMPLES "Build the SDL3_net sample program(s)" ${SDLNET_SAMPLES_DEFAULT})
cmake_dependent_option(SDLNET_SAMPLES_INSTALL "Install the SDL3_net sample program(s)" OFF "SDLNET_SAMPLES;SDLNET_INSTALL" OFF)

# Save BUILD_SHARED_LIBS variable
set(SDLNET_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

set(sdl_required_components Headers)

if(SDLNET_BUILD_SHARED_LIBS)
    set(sdl3_net_target_name SDL3_net-shared)
    set(sdl3_target_name SDL3::SDL3-shared)

    list(APPEND sdl_required_components SDL3-shared)
else()
    set(sdl3_net_target_name SDL3_net-static)
    set(sdl3_target_name SDL3::SDL3)
endif()

if(NOT TARGET SDL3::Headers OR NOT TARGET ${sdl3_target_name})
    find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED COMPONENTS ${sdl_required_components})
endif()

set(PC_LIBS)
set(PC_REQUIRES)

add_library(${sdl3_net_target_name} src/SDL_net.c)
add_library(SDL3_net::${sdl3_net_target_name} ALIAS ${sdl3_net_target_name})
set_property(TARGET ${sdl3_net_target_name} PROPERTY C_STANDARD 99)
if(NOT TARGET SDL3_net::SDL3_net)
    add_library(SDL3_net::SDL3_net ALIAS ${sdl3_net_target_name})
endif()
target_include_directories(${sdl3_net_target_name}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_compile_definitions(${sdl3_net_target_name} PRIVATE
    BUILD_SDL
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
)
target_link_libraries(${sdl3_net_target_name} PUBLIC SDL3::Headers)
if(SDLNET_BUILD_SHARED_LIBS)
    target_link_libraries(${sdl3_net_target_name} PRIVATE SDL3::SDL3-shared)
endif()
sdl_add_warning_options(${sdl3_net_target_name} WARNING_AS_ERROR ${SDLNET_WERROR})
if(WIN32)
    if(SDLNET_BUILD_SHARED_LIBS)
        target_sources(${sdl3_net_target_name} PRIVATE
            src/version.rc
        )
        if(MINGW)
            target_link_options(${sdl3_net_target_name} PRIVATE -static-libgcc)
        endif()
    endif()
    target_link_libraries(${sdl3_net_target_name} PRIVATE iphlpapi ws2_32)
    list(APPEND PC_LIBS -liphlpapi -lws2_32)
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Haiku.*")
    target_link_libraries(${sdl3_net_target_name} PRIVATE network)
endif()
set_target_properties(${sdl3_net_target_name} PROPERTIES
    OUTPUT_NAME "SDL3_net"
    DEFINE_SYMBOL DLL_EXPORT
    EXPORT_NAME ${sdl3_net_target_name}
    C_VISIBILITY_PRESET "hidden"
)

sdl_target_link_option_version_file(${sdl3_net_target_name} "${CMAKE_CURRENT_SOURCE_DIR}/src/SDL_net.sym")

if(NOT ANDROID)
    set_target_properties(${sdl3_net_target_name} PROPERTIES
        SOVERSION "${SO_VERSION_MAJOR}"
        VERSION "${SO_VERSION}"
    )
    if(APPLE)
        cmake_minimum_required(VERSION 3.17...3.28)
        set_target_properties(${sdl3_net_target_name} PROPERTIES
            MACHO_COMPATIBILITY_VERSION "${DYLIB_COMPAT_VERSION}"
            MACHO_CURRENT_VERSION "${DYLIB_CURRENT_VERSION}"
        )
        sdl_no_deprecated_errors(${sdl3_net_target_name})
    endif()
endif()
if(SDLNET_BUILD_SHARED_LIBS)
    if(WIN32)
        set_target_properties(${sdl3_net_target_name} PROPERTIES
            PREFIX ""
        )
    endif()
else()
    if(MSVC)
        set_target_properties(${sdl3_net_target_name} PROPERTIES
            OUTPUT_NAME "SDL3_net-static"
        )
    endif()
endif()

# Use `Compatible Interface Properties` to ensure a shared SDL3_net is built with a shared SDL3
if(SDLNET_BUILD_SHARED_LIBS)
    set_property(TARGET ${sdl3_net_target_name} PROPERTY INTERFACE_SDL3_SHARED TRUE)
    set_property(TARGET ${sdl3_net_target_name} APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL3_SHARED)
endif()

if(SDLNET_BUILD_SHARED_LIBS)
    sdl_target_link_options_no_undefined(${sdl3_net_target_name})
endif()

if(SDLNET_INSTALL)
    install(
        TARGETS ${sdl3_net_target_name}
        EXPORT SDL3NetExports
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3_net/SDL_net.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3_net" COMPONENT devel
    )

    if(WIN32 AND NOT MINGW)
        set(SDLNET_INSTALL_CMAKEDIR_ROOT_DEFAULT "cmake")
    else()
        set(SDLNET_INSTALL_CMAKEDIR_ROOT_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake")
    endif()
    set(SDLNET_INSTALL_CMAKEDIR_ROOT "${SDLNET_INSTALL_CMAKEDIR_ROOT_DEFAULT}" CACHE STRING "Root folder where to install SDL3_netConfig.cmake related files (SDL3_net subfolder for MSVC projects)")
    set(SDLNET_PKGCONFIG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

    if(WIN32 AND NOT MINGW)
        set(SDLNET_INSTALL_CMAKEDIR "${SDLNET_INSTALL_CMAKEDIR_ROOT}")
        set(LICENSES_PREFIX "licenses/SDL3_net")
    else()
        set(SDLNET_INSTALL_CMAKEDIR "${SDLNET_INSTALL_CMAKEDIR_ROOT}/SDL3_net")
        set(LICENSES_PREFIX "${CMAKE_INSTALL_DATAROOTDIR}/licenses/SDL3_net")
    endif()

    configure_package_config_file(cmake/SDL3_netConfig.cmake.in SDL3_netConfig.cmake
        INSTALL_DESTINATION "${SDLNET_INSTALL_CMAKEDIR}"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL3_netConfigVersion.cmake"
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_netConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_netConfigVersion.cmake"
        DESTINATION "${SDLNET_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )
    install(EXPORT SDL3NetExports
        FILE ${sdl3_net_target_name}-targets.cmake
        NAMESPACE SDL3_net::
        DESTINATION "${SDLNET_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )

    if(SDLNET_RELOCATABLE)
        file(RELATIVE_PATH SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${CMAKE_INSTALL_PREFIX}/${SDLNET_PKGCONFIG_INSTALLDIR}" "${CMAKE_INSTALL_PREFIX}")
        string(REGEX REPLACE "[/]+$" "" SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
        set(SDL_PKGCONFIG_PREFIX "\${pcfiledir}/${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
    else()
        set(SDL_PKGCONFIG_PREFIX "${CMAKE_PREFIX_PATH}")
    endif()

    if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
        set(INCLUDEDIR_FOR_PKG_CONFIG "${CMAKE_INSTALL_INCLUDEDIR}")
    else()
        set(INCLUDEDIR_FOR_PKG_CONFIG "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    endif()
    if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(LIBDIR_FOR_PKG_CONFIG "${CMAKE_INSTALL_LIBDIR}")
    else()
        set(LIBDIR_FOR_PKG_CONFIG "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
    endif()

    string(JOIN " " PC_REQUIRES ${PC_REQUIRES})
    string(JOIN " " PC_LIBS ${PC_LIBS})
    configure_file(cmake/sdl3-net.pc.in sdl3-net.pc @ONLY)

    # Always install sdl3-net.pc file: libraries might be different between config modes
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl3-net.pc"
        DESTINATION "${SDLNET_PKGCONFIG_INSTALLDIR}" COMPONENT devel)
    if(BUILD_SHARED_LIBS)
        set(pdbdir "${CMAKE_INSTALL_BINDIR}")
    else()
        set(pdbdir "${CMAKE_INSTALL_LIBDIR}")
    endif()
    if(MSVC)
        SDL_install_pdb("${sdl3_net_target_name}" "${pdbdir}")
    endif()

    install(FILES "LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
        COMPONENT library
    )

    export(TARGETS ${sdl3_net_target_name} NAMESPACE "SDL3_net::" FILE "${sdl3_net_target_name}-targets.cmake")

    if(SDLNET_INSTALL_MAN)
        sdl_get_git_revision_hash(SDLNET_REVISION)
        SDL_generate_manpages(
            HEADERS_DIR "${PROJECT_SOURCE_DIR}/include/SDL3_net"
            SYMBOL "NET_Init"
            WIKIHEADERS_PL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-scripts/wikiheaders.pl"
            REVISION "${SDLNET_REVISION}"
        )
    endif()
endif()

if(SDLNET_SAMPLES)
    function(add_sdl_net_example_executable TARGET)
        if(ANDROID)
            add_library(${TARGET} SHARED ${ARGN})
        else()
            add_executable(${TARGET} ${ARGN})
        endif()
        sdl_add_warning_options(${TARGET} WARNING_AS_ERROR ${SDLTTF_WERROR})
        sdl_target_link_options_no_undefined(${TARGET})
        target_link_libraries(${TARGET} PRIVATE SDL3_net::${sdl3_net_target_name})
        target_link_libraries(${TARGET} PRIVATE ${sdl3_target_name})
        set_property(TARGET ${TARGET} PROPERTY C_STANDARD 99)
        set_property(TARGET ${TARGET} PROPERTY C_EXTENSIONS FALSE)

        if(SDLNET_SAMPLES_INSTALL)
            install(TARGETS ${TARGET}
                RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/installed-tests/SDL3_net"
            )
        endif()
    endfunction()
    add_sdl_net_example_executable(voipchat examples/voipchat.c)
    add_sdl_net_example_executable(simple-http-get examples/simple-http-get.c)
    add_sdl_net_example_executable(resolve-hostnames examples/resolve-hostnames.c)
    add_sdl_net_example_executable(get-local-addrs examples/get-local-addrs.c)
    add_sdl_net_example_executable(echo-server examples/echo-server.c)

    # Build at least one example in C90
    set_property(TARGET get-local-addrs PROPERTY C_STANDARD 90)
endif()
