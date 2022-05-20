file(GLOB audio_model_synthesis_source *.cpp *.h)
add_library(AudioModelSynthesis ${audio_model_synthesis_source})
target_link_libraries(AudioModelSynthesis WorldModule)