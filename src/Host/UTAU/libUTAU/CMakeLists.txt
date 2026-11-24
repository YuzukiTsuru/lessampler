add_library(UTAU
        libUTAU.cpp
        libUTAU.h
        PitchBendDecoder.cpp
        PitchBendDecoder.h
        ScaleConvert.cpp
        ScaleConvert.h
        UTAUParameterParser.cpp
        UTAUParameterParser.h
)
target_link_libraries(UTAU PUBLIC lessampler_common)

if (MSVC)
    target_compile_options(UTAU PRIVATE /wd4819)
endif ()
