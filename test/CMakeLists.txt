set(LESSAMPLER_TEST_DATA_DIR "${PROJECT_SOURCE_DIR}/test")

function(lessampler_add_test target label)
    target_link_libraries(${target} PRIVATE lessampler_common Threads::Threads)
    target_compile_definitions(${target} PRIVATE LESSAMPLER_TEST_DATA_DIR="${LESSAMPLER_TEST_DATA_DIR}")
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS "${label}"
            TIMEOUT 180
    )
    if (WIN32 AND LESSAMPLER_ENABLE_NEURAL AND LESSAMPLER_ONNXRUNTIME_DLL_DIR)
        set_tests_properties(${target} PROPERTIES
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${LESSAMPLER_ONNXRUNTIME_DLL_DIR}")
    endif ()
endfunction()

add_executable(PitchBendTest PitchBendTest.cpp)
target_link_libraries(PitchBendTest PRIVATE UTAU)
lessampler_add_test(PitchBendTest unit)

add_executable(ScaleConvertTest ScaleConvertTest.cpp)
target_link_libraries(ScaleConvertTest PRIVATE UTAU)
lessampler_add_test(ScaleConvertTest unit)

add_executable(libUTAUTest libUTAUTest.cpp)
target_link_libraries(libUTAUTest PRIVATE UTAU)
lessampler_add_test(libUTAUTest unit)

add_executable(ConfigLoaderTest ConfigLoaderTest.cpp)
target_link_libraries(ConfigLoaderTest PRIVATE Config)
lessampler_add_test(ConfigLoaderTest unit)

add_executable(LOGTest LOGTest.cpp)
lessampler_add_test(LOGTest unit)

add_executable(TimerTest TimerTest.cpp)
lessampler_add_test(TimerTest unit)

add_executable(WorldModelCacheTest WorldModelCacheTest.cpp)
target_link_libraries(WorldModelCacheTest PRIVATE Vocoder Audio)
lessampler_add_test(WorldModelCacheTest integration)

add_executable(WorldModelCacheVersionTest WorldModelCacheVersionTest.cpp)
target_link_libraries(WorldModelCacheVersionTest PRIVATE Vocoder Audio)
lessampler_add_test(WorldModelCacheVersionTest integration)

add_executable(WorldAnalyzerTest WorldAnalyzerTest.cpp)
target_link_libraries(WorldAnalyzerTest PRIVATE Vocoder Audio)
lessampler_add_test(WorldAnalyzerTest integration)

add_executable(VoicebankModelerTest VoicebankModelerTest.cpp)
target_link_libraries(VoicebankModelerTest PRIVATE Engine Vocoder Audio Config)
lessampler_add_test(VoicebankModelerTest integration)

add_executable(WorldPipelineTest WorldPipelineTest.cpp)
target_link_libraries(WorldPipelineTest PRIVATE
        Vocoder
        Audio
        Config
        Shine
        UTAU
)
lessampler_add_test(WorldPipelineTest integration)

add_executable(WorldModelTest WorldModelTest.cpp)
target_link_libraries(WorldModelTest PRIVATE Vocoder)
lessampler_add_test(WorldModelTest integration)

add_executable(WorldTransformTest WorldTransformTest.cpp)
target_link_libraries(WorldTransformTest PRIVATE Vocoder)
lessampler_add_test(WorldTransformTest unit)

add_executable(AutoAMPTest AutoAMPTest.cpp)
target_link_libraries(AutoAMPTest PRIVATE Vocoder)
lessampler_add_test(AutoAMPTest unit)

add_executable(WorldSynthesizerTest WorldSynthesizerTest.cpp)
target_link_libraries(WorldSynthesizerTest PRIVATE Vocoder)
lessampler_add_test(WorldSynthesizerTest integration)

add_executable(WavIOTest WavIOTest.cpp)
target_link_libraries(WavIOTest PRIVATE Audio)
lessampler_add_test(WavIOTest unit)

add_executable(ShineTest ShineTest.cpp)
target_link_libraries(ShineTest PRIVATE Shine UTAU)
lessampler_add_test(ShineTest unit)

if (LESSAMPLER_ENABLE_NEURAL)
    add_executable(NeuralFeatureTest NeuralFeatureTest.cpp)
    target_link_libraries(NeuralFeatureTest PRIVATE VocoderNeural)
    lessampler_add_test(NeuralFeatureTest unit)
endif ()

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/EndToEndBinary.h"
        CONTENT "#pragma once\n#define LESSAMPLER_BINARY_PATH R\"lessampler($<TARGET_FILE:lessampler>)lessampler\"\n")

add_executable(RenderEngineTest RenderEngineTest.cpp)
target_link_libraries(RenderEngineTest PRIVATE Vocoder Engine Audio Config Shine UTAU)
if (LESSAMPLER_ENABLE_LLSM)
    target_compile_definitions(RenderEngineTest PRIVATE LESSAMPLER_ENABLE_LLSM=1)
    target_link_libraries(RenderEngineTest PRIVATE llsm2)
endif ()
if (LESSAMPLER_ENABLE_NEURAL)
    target_compile_definitions(RenderEngineTest PRIVATE LESSAMPLER_ENABLE_NEURAL=1)
endif ()
lessampler_add_test(RenderEngineTest integration)

add_executable(EndToEndTest EndToEndTest.cpp)
target_link_libraries(EndToEndTest PRIVATE Audio)
target_include_directories(EndToEndTest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(EndToEndTest lessampler)
lessampler_add_test(EndToEndTest integration)
