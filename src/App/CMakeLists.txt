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

if (LESSAMPLER_ENABLE_NEURAL)
    lessampler_copy_onnxruntime_dll(lessampler)
endif ()

install(TARGETS lessampler RUNTIME DESTINATION .)

if (APPLE)
    set_target_properties(lessampler PROPERTIES INSTALL_RPATH "@loader_path")
elseif (UNIX)
    set_target_properties(lessampler PROPERTIES INSTALL_RPATH "$ORIGIN")
endif ()

install(FILES
        "${PROJECT_SOURCE_DIR}/README.md"
        "${PROJECT_SOURCE_DIR}/LICENSE"
        DESTINATION .
)

if (LESSAMPLER_ENABLE_LLSM)
    install(FILES "${PROJECT_SOURCE_DIR}/lib/llsm/LICENSE"
            DESTINATION licenses
            RENAME "GPLv3-llsm2.txt")
    if (UNIX AND NOT APPLE)
        find_file(GOMP_RUNTIME
                NAMES libgomp.so.1
                PATHS /usr/lib /usr/lib64 /lib /lib64
                /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                /lib/${CMAKE_LIBRARY_ARCHITECTURE})
        if (GOMP_RUNTIME)
            get_filename_component(GOMP_RUNTIME_REAL "${GOMP_RUNTIME}" REALPATH)
            install(FILES "${GOMP_RUNTIME_REAL}" DESTINATION . RENAME "libgomp.so.1")
        endif ()
    endif ()
endif ()

if (LESSAMPLER_ENABLE_NEURAL)
    if (WIN32)
        if (LESSAMPLER_ONNXRUNTIME_DLL)
            install(FILES "${LESSAMPLER_ONNXRUNTIME_DLL}" DESTINATION .)
        endif ()
    elseif (APPLE)
        install(FILES "${LESSAMPLER_ONNXRUNTIME_LIBRARY}" DESTINATION .)
    else ()
        install(FILES "${LESSAMPLER_ONNXRUNTIME_LIBRARY}" DESTINATION . RENAME "libonnxruntime.so.1")
    endif ()

    if (LESSAMPLER_ONNXRUNTIME_LICENSE_DIR)
        if (EXISTS "${LESSAMPLER_ONNXRUNTIME_LICENSE_DIR}/LICENSE")
            install(FILES "${LESSAMPLER_ONNXRUNTIME_LICENSE_DIR}/LICENSE"
                    DESTINATION licenses
                    RENAME "MIT-ONNX-Runtime.txt")
        endif ()
        if (EXISTS "${LESSAMPLER_ONNXRUNTIME_LICENSE_DIR}/ThirdPartyNotices.txt")
            install(FILES "${LESSAMPLER_ONNXRUNTIME_LICENSE_DIR}/ThirdPartyNotices.txt"
                    DESTINATION licenses)
        endif ()
    endif ()
endif ()
