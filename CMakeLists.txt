cmake_minimum_required(VERSION 3.16)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)

# use git version as library version
find_package(Git QUIET)
if (Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE _git_version
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
            OUTPUT_VARIABLE PROJECT_GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
else ()
    set(_git_version 0)
    set(PROJECT_GIT_HASH "")
endif ()

project(lessampler LANGUAGES C CXX VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${_git_version})

set(CMAKE_CXX_STANDARD 20)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHsc")
endif ()

# Add support for -DDEBUG_MODE=ON
set(DEBUG_SUFFIX)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    ADD_DEFINITIONS(-DDEBUG_MODE)
    set(DEBUG_SUFFIX "d")
endif ()

# Generate Configure file
configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/lessconfig.h"
)

if (MSVC)
    configure_file(
            "${PROJECT_SOURCE_DIR}/assets/lessampler.rc.in"
            "${PROJECT_BINARY_DIR}/lessampler.rc"
    )
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(Threads REQUIRED)

set(WORLD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WORLD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(INICPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INICPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "" FORCE)
set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "" FORCE)

# Only for rapidjson
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "" FORCE)
set(RAPIDJSON_ENABLE_INSTRUMENTATION_OPT OFF CACHE BOOL "" FORCE)

# subdirectory
add_subdirectory(lib/World EXCLUDE_FROM_ALL)
add_subdirectory(lib/dialog EXCLUDE_FROM_ALL)
add_subdirectory(lib/inicpp EXCLUDE_FROM_ALL)
add_subdirectory(lib/ftxui EXCLUDE_FROM_ALL)
add_subdirectory(lib/rapidjson EXCLUDE_FROM_ALL)
add_subdirectory(lib/lz4/build/cmake EXCLUDE_FROM_ALL)
add_subdirectory(lib/sndfile EXCLUDE_FROM_ALL)

# include header files
include_directories(
        src
        lib/dialog
        lib/ColorCout/includes
        lib/World/src
        lib/World/tools
        lib/ftxui/include
        lib/inicpp/include
        lib/rapidjson/include
        lib/sndfile/include
        lib/lz4/lib
        ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/lib/sndfile/include
)

add_subdirectory(src)
add_subdirectory(test)
