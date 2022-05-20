file(GLOB audio_process_source *.cpp *.h)
add_library(AduioProcess ${audio_process_source})
target_link_libraries(AduioProcess UTAU)