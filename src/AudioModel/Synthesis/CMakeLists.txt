add_library(AudioModelSynthesis
        Synthesis.cpp
        Synthesis.h
)
target_link_libraries(AudioModelSynthesis PUBLIC world::core lessampler_common)
