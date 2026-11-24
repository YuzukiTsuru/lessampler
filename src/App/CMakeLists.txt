add_executable(lessampler
        Application.cpp
        Application.h
        main.cpp
)

if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_sources(lessampler PRIVATE ${PROJECT_BINARY_DIR}/lessampler.rc)
endif ()

set_target_properties(lessampler PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)

target_link_libraries(lessampler PRIVATE
        Core
        Config
        Audio
        Vocoder
        Engine
        Shine
        UTAU
        Platform
        Threads::Threads
)
