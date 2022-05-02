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
set(CMAKE_CXX_STANDARD 17)

# include header files
include_directories(
        src
        src/LOG
        src/Utils
        src/libUTAU
        src/Dialogs
        lib/World/src
        lib/ColorCout/includes
        lib/dialog
        ${PROJECT_BINARY_DIR}
)

# Generate Configure file
configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/config.h"
)

# Library World
set(WORLD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WORLD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/World EXCLUDE_FROM_ALL)

add_subdirectory(lib/dialog)

add_subdirectory(test)

# Library LOG
file(GLOB log_source src/LOG/*.cpp src/LOG/*.h)
add_library(LOG ${log_source})

# Library UTAU
file(GLOB utau_source src/libUTAU/*.cpp src/libUTAU/*.h)
add_library(UTAU ${utau_source})

# Library Dialog
file(GLOB Dialogs_source src/Dialogs/*.cpp src/Dialogs/*.h)
add_library(Dialogs ${Dialogs_source})
target_link_libraries(Dialogs LOG)

# Library Audio Model
file(GLOB audio_model_source src/AudioModel/*.cpp src/AudioModel/*.h src/AudioModel/WorldModule/*.cpp src/AudioModel/WorldModule/*.h)
add_library(AudioModel ${audio_model_source})
target_link_libraries(AudioModel LOG world::core)

file(GLOB lessampler_source src/*.cpp)
add_executable(lessampler ${lessampler_source})
target_link_libraries(lessampler AudioModel LOG Dialogs UTAU)
