# Library Audio Model

add_subdirectory(WorldModule)

file(GLOB audio_model_source *.cpp *.h)
add_library(AudioModel ${audio_model_source})
target_link_libraries(AudioModel WorldModule)