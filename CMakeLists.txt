cmake_minimum_required(VERSION 3.30...4.2)

project(lessampler LANGUAGES C CXX)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# The short commit hash is shown in the banner and stored in the config file.
find_package(Git QUIET)
if (Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE PROJECT_GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
else ()
    set(PROJECT_GIT_HASH "")
endif ()

# C++26 requires GCC 15+, Clang 19+, or an MSVC toolchain driven in /std:c++latest mode.
if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # CMake does not map CXX_STANDARD 26 to MSVC yet. CXX_STANDARD 23 selects
    # /std:c++latest, which is the mode we want for the latest MSVC features.
    set(CMAKE_CXX_STANDARD 23)
else ()
    set(CMAKE_CXX_STANDARD 26)
endif ()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15)
    message(FATAL_ERROR "lessampler requires GCC 15 or newer, found ${CMAKE_CXX_COMPILER_VERSION}")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19)
    message(FATAL_ERROR "lessampler requires Clang 19 or newer, found ${CMAKE_CXX_COMPILER_VERSION}")
endif ()

if (MSVC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        configure_file(
                "${PROJECT_SOURCE_DIR}/assets/lessampler.rc.in"
                "${PROJECT_BINARY_DIR}/lessampler.rc"
        )
    endif ()
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Project options
option(LESSAMPLER_BUILD_TESTS "Build the test executables and register them with CTest" ON)
option(LESSAMPLER_BUILD_TOOLS "Build helper tools" ON)
option(LESSAMPLER_ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(LESSAMPLER_ENABLE_LLSM "Enable the GPLv3 libllsm2 vocoder backend" OFF)
option(LESSAMPLER_ENABLE_NEURAL "Enable the optional ONNX Runtime neural vocoder backend" OFF)
option(LESSAMPLER_DUMP_DATA "Enable verbose per-sample debug dumps" OFF)
option(LESSAMPLER_TIME_EVAL "Print timing evaluation messages" OFF)
option(LESSAMPLER_WERROR "Treat compiler warnings as errors" OFF)

include("${PROJECT_SOURCE_DIR}/cmake/OnnxRuntime.cmake")

# Common settings shared by every first-party target.
add_library(lessampler_common INTERFACE)
target_include_directories(lessampler_common INTERFACE
        "${PROJECT_SOURCE_DIR}/src"
        "${PROJECT_BINARY_DIR}"
)
if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_features(lessampler_common INTERFACE cxx_std_23)
else ()
    target_compile_features(lessampler_common INTERFACE cxx_std_26)
endif ()
target_compile_definitions(lessampler_common INTERFACE $<$<CONFIG:Debug>:DEBUG_MODE=1>)

if (WIN32)
    # windows.h defines min/max as macros, which breaks the std::min/std::max
    # calls used throughout the codebase.
    target_compile_definitions(lessampler_common INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN)
endif ()

if (LESSAMPLER_DUMP_DATA)
    target_compile_definitions(lessampler_common INTERFACE LESSAMPLER_DUMP_DATA=1)
endif ()
if (LESSAMPLER_TIME_EVAL)
    target_compile_definitions(lessampler_common INTERFACE LESSAMPLER_TIME_EVAL=1)
endif ()

if (MSVC)
    target_compile_options(lessampler_common INTERFACE /W4 /utf-8 /permissive-)
else ()
    target_compile_options(lessampler_common INTERFACE
            -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
    )
endif ()

if (LESSAMPLER_ENABLE_LLSM)
    message(STATUS "LLSM backend enabled; the resulting lessampler binary is GPLv3")
endif ()
if (LESSAMPLER_ENABLE_NEURAL)
    message(STATUS "Neural backend enabled; ONNX Runtime (MIT) is downloaded at configure time")
endif ()

if (LESSAMPLER_WERROR)
    if (MSVC)
        target_compile_options(lessampler_common INTERFACE /WX)
    else ()
        target_compile_options(lessampler_common INTERFACE -Werror)
    endif ()
endif ()

if (LESSAMPLER_ENABLE_SANITIZERS)
    if (MSVC)
        target_compile_options(lessampler_common INTERFACE /fsanitize=address)
    else ()
        target_compile_options(lessampler_common INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(lessampler_common INTERFACE -fsanitize=address,undefined)
    endif ()
endif ()

# Third-party options. Keep them scoped to the subprojects that read them.
set(WORLD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WORLD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "" FORCE)
set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

set(tomlplusplus_INSTALL OFF CACHE BOOL "" FORCE)
set(tomlplusplus_BUILD_LIB OFF CACHE BOOL "" FORCE)
set(tomlplusplus_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(tomlplusplus_BUILD_TOML_TEST OFF CACHE BOOL "" FORCE)
set(tomlplusplus_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(TOMLPLUSPLUS_BUILD_MODULES OFF CACHE BOOL "" FORCE)
set(tomlplusplus_REGENERATE OFF CACHE BOOL "" FORCE)

# CMake 4 rejects third-party projects that still declare cmake_minimum_required < 3.5.
# Setting this only around add_subdirectory() keeps our own policy version intact.
if (CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif ()

add_subdirectory(lib/World EXCLUDE_FROM_ALL)
add_subdirectory(lib/dialog EXCLUDE_FROM_ALL)
add_subdirectory(lib/sndfile EXCLUDE_FROM_ALL)
add_subdirectory(lib/tomlplusplus EXCLUDE_FROM_ALL)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
    unset(CMAKE_POLICY_VERSION_MINIMUM)
endif ()

# Generate lessconfig.h before any target compiles.
configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/lessconfig.h"
)

add_subdirectory(src)

if (LESSAMPLER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

if (LESSAMPLER_BUILD_TOOLS)
    add_subdirectory(tools)
endif ()
