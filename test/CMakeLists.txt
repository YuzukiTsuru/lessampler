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

add_executable(EnvelopeTest EnvelopeTest.cpp)
target_link_libraries(EnvelopeTest PRIVATE Wavtool)
lessampler_add_test(EnvelopeTest unit)

add_executable(WavAssemblyTest WavAssemblyTest.cpp)
target_link_libraries(WavAssemblyTest PRIVATE Wavtool)
lessampler_add_test(WavAssemblyTest unit)

add_executable(MasterChainTest MasterChainTest.cpp)
target_link_libraries(MasterChainTest PRIVATE Audio)
lessampler_add_test(MasterChainTest unit)

add_executable(GrowlShaperTest GrowlShaperTest.cpp)
target_link_libraries(GrowlShaperTest PRIVATE Audio Core)
lessampler_add_test(GrowlShaperTest unit)

add_executable(WavtoolProtocolTest WavtoolProtocolTest.cpp)
target_link_libraries(WavtoolProtocolTest PRIVATE Wavtool)
lessampler_add_test(WavtoolProtocolTest unit)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/WavtoolCliPaths.h"
        CONTENT "#pragma once\n#define LESSAMPLER_BINARY_PATH R\"ls($<TARGET_FILE:lessampler>)ls\"\n")

add_executable(WavtoolCliTest WavtoolCliTest.cpp)
target_link_libraries(WavtoolCliTest PRIVATE Wavtool)
target_include_directories(WavtoolCliTest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(WavtoolCliTest lessampler)
lessampler_add_test(WavtoolCliTest integration)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/WavtoolEndToEndPaths.h"
        CONTENT "#pragma once\n#define LESSAMPLER_BINARY_PATH R\"ls($<TARGET_FILE:lessampler>)ls\"\n")

add_executable(WavtoolEndToEndTest WavtoolEndToEndTest.cpp)
target_link_libraries(WavtoolEndToEndTest PRIVATE Wavtool)
target_include_directories(WavtoolEndToEndTest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(WavtoolEndToEndTest lessampler)
lessampler_add_test(WavtoolEndToEndTest integration)

add_executable(ShineTest ShineTest.cpp)
target_link_libraries(ShineTest PRIVATE Shine UTAU)
lessampler_add_test(ShineTest unit)

add_executable(PresetFactoryTest PresetFactoryTest.cpp)
target_link_libraries(PresetFactoryTest PRIVATE Config Audio)
lessampler_add_test(PresetFactoryTest unit)

add_executable(VoiceStyleTest VoiceStyleTest.cpp)
target_link_libraries(VoiceStyleTest PRIVATE Core)
lessampler_add_test(VoiceStyleTest unit)

add_executable(F0EstimatorTest F0EstimatorTest.cpp)
target_link_libraries(F0EstimatorTest PRIVATE VocoderCommon Audio Config)
lessampler_add_test(F0EstimatorTest integration)

if (LESSAMPLER_ENABLE_NEURAL)
    add_executable(NeuralFeatureTest NeuralFeatureTest.cpp)
    target_link_libraries(NeuralFeatureTest PRIVATE VocoderNeural)
    lessampler_add_test(NeuralFeatureTest unit)

    add_executable(PolishTest PolishTest.cpp)
    target_link_libraries(PolishTest PRIVATE Vocoder)
    lessampler_add_test(PolishTest unit)

    add_executable(FeatureAssemblerTest FeatureAssemblerTest.cpp)
    target_link_libraries(FeatureAssemblerTest PRIVATE Vocoder)
    lessampler_add_test(FeatureAssemblerTest unit)
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

find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_Interpreter_FOUND)
    add_test(NAME ExpressionTest
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/test_expression.py")
    set_tests_properties(ExpressionTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS "unit"
            TIMEOUT 60
    )

    add_test(NAME ConfiguratorTest
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/test_lessampler_config.py")
    set_tests_properties(ConfiguratorTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS "unit"
            TIMEOUT 60
    )

    set(LESSAMPLER_QUALITY_EVAL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/quality/evaluate.py")
    set(LESSAMPLER_QUALITY_FIXTURE "${CMAKE_CURRENT_SOURCE_DIR}/quality/fixture.ust")

    add_test(NAME QualityEvaluationTest
            COMMAND "${Python3_EXECUTABLE}" "${LESSAMPLER_QUALITY_EVAL_SCRIPT}"
                    --mos none
                    --ust "${LESSAMPLER_QUALITY_FIXTURE}"
                    --lessampler "$<TARGET_FILE:lessampler>"
                    --json "${CMAKE_CURRENT_BINARY_DIR}/quality-report.json"
                    "${LESSAMPLER_TEST_DATA_DIR}/ne.wav")
    set_tests_properties(QualityEvaluationTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS "evaluation"
            TIMEOUT 180
    )

    if (LESSAMPLER_ENABLE_SINGMOS_TESTS)
        if (LESSAMPLER_SINGMOS_PYTHON)
            set(LESSAMPLER_SINGMOS_INTERPRETER "${LESSAMPLER_SINGMOS_PYTHON}")
        else ()
            set(LESSAMPLER_SINGMOS_INTERPRETER "${Python3_EXECUTABLE}")
        endif ()
        add_test(NAME SingMOSEvaluationTest
                COMMAND "${LESSAMPLER_SINGMOS_INTERPRETER}" "${LESSAMPLER_QUALITY_EVAL_SCRIPT}"
                        --mos singmos
                        --ust "${LESSAMPLER_QUALITY_FIXTURE}"
                        --lessampler "$<TARGET_FILE:lessampler>"
                        --json "${CMAKE_CURRENT_BINARY_DIR}/singmos-report.json"
                        "${LESSAMPLER_TEST_DATA_DIR}/ne.wav")
        set_tests_properties(SingMOSEvaluationTest PROPERTIES
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                LABELS "evaluation;singmos"
                TIMEOUT 900
        )
    endif ()
else ()
    message(STATUS "Python 3 not found; quality evaluation tests disabled")
endif ()
