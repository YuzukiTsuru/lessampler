add_subdirectory(WorldModule)
add_subdirectory(Synthesis)

add_library(AudioModel
        AudioModel.cpp
        AudioModel.h
        lessAudioModel.h
)
target_link_libraries(AudioModel PUBLIC
        WorldModule
        AudioModelSynthesis
        lessampler_common
)
