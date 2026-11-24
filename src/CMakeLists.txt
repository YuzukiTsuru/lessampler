add_subdirectory(AudioModel)
add_subdirectory(ConfigUnit)
add_subdirectory(Dialogs)
add_subdirectory(FileIO)
add_subdirectory(Shine)
add_subdirectory(AudioProcess)

add_executable(lessampler main.cpp lessampler.cpp lessampler.h)

if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_sources(lessampler PRIVATE ${PROJECT_BINARY_DIR}/lessampler.rc)
endif ()

target_link_libraries(lessampler PRIVATE
        AudioModel
        AudioProcess
        FileIO
        Dialogs
        UTAU
        ConfigUnit
        lessampler_common
        Threads::Threads
)
