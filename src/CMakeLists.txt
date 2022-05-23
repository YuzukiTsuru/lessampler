add_subdirectory(AudioModel)
add_subdirectory(ConfigUnit)
add_subdirectory(Dialogs)
add_subdirectory(FileIO)
add_subdirectory(libUTAU)
add_subdirectory(AudioProcess)

file(GLOB lessampler_source *.cpp *.h)

if (MSVC)
    set(lessampler_source ${lessampler_source} ${RC_ICON})
endif ()

add_executable(lessampler ${lessampler_source})
target_link_libraries(lessampler AudioModel FileIO Dialogs UTAU ConfigUnit)

if (MSVC)
    target_compile_options(lessampler PUBLIC "/utf-8")
endif ()