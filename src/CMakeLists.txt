add_subdirectory(AudioModel)
add_subdirectory(ConfigUnit)
add_subdirectory(Dialogs)
add_subdirectory(FileIO)
add_subdirectory(libUTAU)

file(GLOB lessampler_source *.cpp *.h)
add_executable(lessampler ${lessampler_source})
target_link_libraries(lessampler AudioModel Dialogs UTAU ConfigUnit)