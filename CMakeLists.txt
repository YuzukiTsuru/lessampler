cmake_minimum_required(VERSION 3.16)

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

project(lessampler LANGUAGES C CXX VERSION ${_git_version})
set(CMAKE_CXX_STANDARD 20)

# Add support for -DDEBUG_MODE=ON
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    ADD_DEFINITIONS(-DDEBUG_MODE)
endif ()

# include header files
include_directories(
        src
        src/AudioModel
        src/LOG
        src/Utils
        src/libUTAU
        src/Dialogs
        lib/World/src
        lib/ColorCout/includes
        lib/dialog
        lib/ftxui/include
        lib/inicpp/include
        lib/rapidjson/include
        lib/sndfile/include
        ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/lib/sndfile/include
)

# Generate Configure file
configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/config.h"
)

# Library World
set(WORLD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WORLD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(INICPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INICPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "" FORCE)
set(RAPIDJSON_ENABLE_INSTRUMENTATION_OPT OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "" FORCE)
set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "" FORCE)

# subdirectory
add_subdirectory(lib/World EXCLUDE_FROM_ALL)
add_subdirectory(lib/dialog EXCLUDE_FROM_ALL)
add_subdirectory(lib/inicpp EXCLUDE_FROM_ALL)
add_subdirectory(lib/ftxui EXCLUDE_FROM_ALL)
add_subdirectory(lib/rapidjson EXCLUDE_FROM_ALL)
add_subdirectory(lib/sndfile EXCLUDE_FROM_ALL)

# test
add_subdirectory(test)

# Library UTAU
file(GLOB utau_source src/libUTAU/*.cpp src/libUTAU/*.h)
add_library(UTAU ${utau_source})

# Library Dialog
file(GLOB Dialogs_source src/Dialogs/*.cpp src/Dialogs/*.h)
add_library(Dialogs ${Dialogs_source})
target_link_libraries(Dialogs)

# Library FileIO
file(GLOB FileIO_source src/FileIO/*.cpp src/FileIO/*.h)
add_library(FileIO ${FileIO_source})
target_link_libraries(FileIO rapidjson)

# Library Audio Model
file(GLOB audio_model_source src/AudioModel/*.cpp src/AudioModel/*.h src/AudioModel/WorldModule/*.cpp src/AudioModel/WorldModule/*.h)
add_library(AudioModel ${audio_model_source})
target_link_libraries(AudioModel world::core)

# Library ConfigUnit
file(GLOB config_source src/ConfigUnit/*.cpp src/ConfigUnit/*.h)
add_library(Config ${config_source})

# Main
file(GLOB lessampler_source src/*.cpp)
add_executable(lessampler ${lessampler_source})
target_link_libraries(lessampler AudioModel Dialogs UTAU)
