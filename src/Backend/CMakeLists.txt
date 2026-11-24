add_library(Backend
        VocoderBackend.cpp
        VocoderBackend.h
        VocoderKind.h
        WorldBackend.cpp
        WorldBackend.h
)
target_link_libraries(Backend PUBLIC
        lessampler_common
        ConfigUnit
        FileIO
        AudioProcess
        AudioModel
        Shine
        UTAU
)

if (LESSAMPLER_ENABLE_LLSM)
    set(CIGLET_DIR "${PROJECT_SOURCE_DIR}/lib/ciglet")
    set(LLSM_DIR "${PROJECT_SOURCE_DIR}/lib/llsm")

    add_library(ciglet STATIC
            "${CIGLET_DIR}/ciglet.c"
            "${CIGLET_DIR}/external/fftsg_h.c"
            "${CIGLET_DIR}/external/fast_median.c"
    )
    target_include_directories(ciglet PUBLIC
            "${CIGLET_DIR}"
            "${CIGLET_DIR}/external"
    )
    target_compile_definitions(ciglet PUBLIC FP_TYPE=float)
    target_compile_features(ciglet PRIVATE c_std_99)
    set_target_properties(ciglet PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if (MSVC)
        target_compile_options(ciglet PRIVATE /w)
        target_compile_definitions(ciglet PRIVATE _USE_MATH_DEFINES)
    else ()
        target_compile_options(ciglet PRIVATE -w)
        target_compile_definitions(ciglet PRIVATE _POSIX_C_SOURCE=2)
    endif ()

    add_library(llsm2 STATIC
            "${LLSM_DIR}/container.c"
            "${LLSM_DIR}/frame.c"
            "${LLSM_DIR}/dsputils.c"
            "${LLSM_DIR}/llsmutils.c"
            "${LLSM_DIR}/layer0.c"
            "${LLSM_DIR}/layer1.c"
    )
    # libllsm2 includes <ciglet/ciglet.h>; the parent of lib/ciglet is lib/.
    # The compatibility directory supplies an empty <libihnm/ihnm.h> because
    # layer0.c includes it without using any of its symbols.
    target_include_directories(llsm2 PUBLIC
            "${LLSM_DIR}"
            "${PROJECT_SOURCE_DIR}/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/compat"
    )
    target_compile_definitions(llsm2 PUBLIC FP_TYPE=float)
    target_compile_features(llsm2 PRIVATE c_std_99)
    set_target_properties(llsm2 PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(llsm2 PUBLIC ciglet Threads::Threads)
    if (MSVC)
        target_compile_options(llsm2 PRIVATE /w)
        target_compile_definitions(llsm2 PRIVATE _USE_MATH_DEFINES)
    else ()
        target_compile_options(llsm2 PRIVATE -w)
        target_compile_definitions(llsm2 PRIVATE _POSIX_C_SOURCE=2)
        target_link_libraries(llsm2 PUBLIC m)
    endif ()

    find_package(OpenMP QUIET)
    if (OpenMP_C_FOUND)
        target_link_libraries(ciglet PUBLIC OpenMP::OpenMP_C)
        target_link_libraries(llsm2 PUBLIC OpenMP::OpenMP_C)
    endif ()

    # lessampler_common only instruments the C++ targets. Add the same
    # sanitizer instrumentation to the vendored C sources so a sanitizer build
    # also covers libllsm2 and ciglet.
    if (LESSAMPLER_ENABLE_SANITIZERS)
        if (MSVC)
            target_compile_options(ciglet PRIVATE /fsanitize=address)
            target_compile_options(llsm2 PRIVATE /fsanitize=address)
        else ()
            target_compile_options(ciglet PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_compile_options(llsm2 PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        endif ()
    endif ()

    target_sources(Backend PRIVATE LlsmBackend.cpp LlsmBackend.h)
    target_link_libraries(Backend PRIVATE llsm2)
    target_compile_definitions(Backend PRIVATE LESSAMPLER_ENABLE_LLSM=1)
endif ()
