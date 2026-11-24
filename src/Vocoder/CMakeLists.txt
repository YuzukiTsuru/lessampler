add_library(VocoderCommon STATIC
        Common/F0Estimator.cpp
        Common/F0Estimator.h
)
target_link_libraries(VocoderCommon PUBLIC
        Core
        Config
        world::core
)

add_library(VocoderWorld STATIC
        World/WorldAnalyzer.cpp
        World/WorldAnalyzer.h
        World/WorldBackend.cpp
        World/WorldBackend.h
        World/WorldModel.h
        World/WorldModelBuilder.cpp
        World/WorldModelBuilder.h
        World/WorldModelCache.cpp
        World/WorldModelCache.h
        World/WorldModelStore.cpp
        World/WorldModelStore.h
        World/WorldPara.h
        World/WorldSynthesizer.cpp
        World/WorldSynthesizer.h
        World/WorldTransform.cpp
        World/WorldTransform.h
)
target_link_libraries(VocoderWorld PUBLIC
        VocoderCommon
        Core
        Config
        Audio
        world::core
        Threads::Threads
)

if (LESSAMPLER_ENABLE_NEURAL)
    lessampler_setup_onnxruntime()

    add_library(VocoderNeural STATIC
            Neural/MelSpectrogram.cpp
            Neural/MelSpectrogram.h
            Neural/NeuralBackend.cpp
            Neural/NeuralBackend.h
            Neural/NeuralModel.cpp
            Neural/NeuralModel.h
    )
    lessampler_link_onnxruntime(VocoderNeural)
    target_link_libraries(VocoderNeural PUBLIC
            VocoderWorld
            Core
            Config
            Audio
    )
    target_compile_definitions(VocoderNeural PRIVATE LESSAMPLER_ENABLE_NEURAL=1)
endif ()

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
    # The compatibility directory supplies an empty <libihnm/ihnm.h>.
    target_include_directories(llsm2 PUBLIC
            "${LLSM_DIR}"
            "${PROJECT_SOURCE_DIR}/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/Llsm/compat"
            "${CMAKE_CURRENT_SOURCE_DIR}/Llsm"
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
    # MSVC's OpenMP front end rejects the induction-variable form used by
    # ciglet. Build that C code single-threaded on MSVC rather than patching
    # the vendored source.
    if (OpenMP_C_FOUND AND NOT MSVC)
        target_link_libraries(ciglet PUBLIC OpenMP::OpenMP_C)
        target_link_libraries(llsm2 PUBLIC OpenMP::OpenMP_C)
    endif ()

    if (LESSAMPLER_ENABLE_SANITIZERS)
        if (MSVC)
            target_compile_options(ciglet PRIVATE /fsanitize=address)
            target_compile_options(llsm2 PRIVATE /fsanitize=address)
        else ()
            target_compile_options(ciglet PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_compile_options(llsm2 PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        endif ()
    endif ()

    add_library(VocoderLlsm STATIC
            Llsm/LlsmBackend.cpp
            Llsm/LlsmBackend.h
            Llsm/LlsmCache.cpp
            Llsm/LlsmCache.h
    )
    target_link_libraries(VocoderLlsm PUBLIC
            VocoderCommon
            Core
            Config
            Audio
    )
    target_link_libraries(VocoderLlsm PRIVATE llsm2)
    target_compile_definitions(VocoderLlsm PRIVATE LESSAMPLER_ENABLE_LLSM=1)
endif ()

add_library(Vocoder STATIC
        VocoderBackend.cpp
        VocoderBackend.h
)
target_link_libraries(Vocoder PUBLIC
        VocoderCommon
        VocoderWorld
)
if (LESSAMPLER_ENABLE_LLSM)
    target_link_libraries(Vocoder PUBLIC VocoderLlsm)
    target_compile_definitions(Vocoder PRIVATE LESSAMPLER_ENABLE_LLSM=1)
endif ()
if (LESSAMPLER_ENABLE_NEURAL)
    target_link_libraries(Vocoder PUBLIC VocoderNeural)
    target_compile_definitions(Vocoder PRIVATE LESSAMPLER_ENABLE_NEURAL=1)
endif ()
