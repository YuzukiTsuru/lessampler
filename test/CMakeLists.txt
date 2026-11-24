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

add_executable(ConfigUnitTest ConfigUnitTest.cpp)
target_link_libraries(ConfigUnitTest PRIVATE Config)
lessampler_add_test(ConfigUnitTest unit)

add_executable(LOGTest LOGTest.cpp)
lessampler_add_test(LOGTest unit)

add_executable(TimerTest TimerTest.cpp)
lessampler_add_test(TimerTest unit)

add_executable(AudioModelBinTest AudioModelBinTest.cpp)
target_link_libraries(AudioModelBinTest PRIVATE Analysis Audio)
lessampler_add_test(AudioModelBinTest integration)

add_executable(AudioModelBinVersionTest AudioModelBinVersionTest.cpp)
target_link_libraries(AudioModelBinVersionTest PRIVATE Analysis Audio)
lessampler_add_test(AudioModelBinVersionTest integration)

add_executable(WorldModuleTest WorldModuleTest.cpp)
target_link_libraries(WorldModuleTest PRIVATE Analysis Audio)
lessampler_add_test(WorldModuleTest integration)

add_executable(GenerateAudioModelTest GenerateAudioModelTest.cpp)
target_link_libraries(GenerateAudioModelTest PRIVATE Engine Analysis Audio Config)
lessampler_add_test(GenerateAudioModelTest integration)

add_executable(AudioProcessTest AudioProcessTest.cpp)
target_link_libraries(AudioProcessTest PRIVATE
        Analysis
        Audio
        Config
        Shine
        UTAU
)
lessampler_add_test(AudioProcessTest integration)

add_executable(AudioModelTest AudioModelTest.cpp)
target_link_libraries(AudioModelTest PRIVATE Analysis)
lessampler_add_test(AudioModelTest integration)

add_executable(AudioProcessUnitTest AudioProcessUnitTest.cpp)
target_link_libraries(AudioProcessUnitTest PRIVATE Analysis)
lessampler_add_test(AudioProcessUnitTest unit)

add_executable(AutoAMPTest AutoAMPTest.cpp)
target_link_libraries(AutoAMPTest PRIVATE Analysis)
lessampler_add_test(AutoAMPTest unit)

add_executable(SynthesisTest SynthesisTest.cpp)
target_link_libraries(SynthesisTest PRIVATE Analysis)
lessampler_add_test(SynthesisTest integration)

add_executable(WavIOTest WavIOTest.cpp)
target_link_libraries(WavIOTest PRIVATE Audio)
lessampler_add_test(WavIOTest unit)

add_executable(ShineTest ShineTest.cpp)
target_link_libraries(ShineTest PRIVATE Shine UTAU)
lessampler_add_test(ShineTest unit)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/EndToEndBinary.h"
        CONTENT "#pragma once\n#define LESSAMPLER_BINARY_PATH R\"lessampler($<TARGET_FILE:lessampler>)lessampler\"\n")

add_executable(BackendTest BackendTest.cpp)
target_link_libraries(BackendTest PRIVATE Backend Engine Analysis Audio Config Shine UTAU)
if (LESSAMPLER_ENABLE_LLSM)
    target_compile_definitions(BackendTest PRIVATE LESSAMPLER_ENABLE_LLSM=1)
    target_link_libraries(BackendTest PRIVATE llsm2)
endif ()
lessampler_add_test(BackendTest integration)

add_executable(EndToEndTest EndToEndTest.cpp)
target_link_libraries(EndToEndTest PRIVATE Audio)
target_include_directories(EndToEndTest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(EndToEndTest lessampler)
lessampler_add_test(EndToEndTest integration)
