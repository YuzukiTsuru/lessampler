file(GLOB audio_process_source *.cpp *.h)
add_library(AudioProcess ${audio_process_source})
target_link_libraries(AudioProcess UTAU Threads::Threads)