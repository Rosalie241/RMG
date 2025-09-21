# This cmake build script is meant for verifying the various CMake configuration script.

cmake_minimum_required(VERSION 3.12...3.28)
project(sdl_test LANGUAGES C)

cmake_policy(SET CMP0074 NEW)

# Override CMAKE_FIND_ROOT_PATH_MODE to allow search for SDL3 outside of sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

include(FeatureSummary)

option(TEST_SHARED "Test linking to shared SDL3_net library" ON)
add_feature_info("TEST_SHARED" TEST_SHARED "Test linking with shared library")

option(TEST_STATIC "Test linking to static SDL3_net library" ON)
add_feature_info("TEST_STATIC" TEST_STATIC "Test linking with static library")

if(ANDROID)
    macro(add_executable NAME)
        set(args ${ARGN})
        list(REMOVE_ITEM args WIN32)
        add_library(${NAME} SHARED ${args})
        unset(args)
    endmacro()
endif()

if(TEST_SHARED)
    find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)
    find_package(SDL3_net REQUIRED CONFIG)
    add_executable(main_shared main.c)
    target_link_libraries(main_shared PRIVATE SDL3_net::SDL3_net-shared SDL3::SDL3)
endif()

if(TEST_STATIC)
    find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)
    # some static vendored libraries use c++ (enable CXX after `find_package` might show a warning)
    enable_language(CXX)
    find_package(SDL3_net REQUIRED CONFIG)
    add_executable(main_static main.c)
    target_link_libraries(main_static PRIVATE SDL3_net::SDL3_net-static SDL3::SDL3)
endif()

feature_summary(WHAT ALL)
