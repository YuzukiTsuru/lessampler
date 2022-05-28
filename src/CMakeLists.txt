add_subdirectory(AudioModel)
add_subdirectory(ConfigUnit)
add_subdirectory(Dialogs)
add_subdirectory(FileIO)
add_subdirectory(libUTAU)
add_subdirectory(AudioProcess)

file(GLOB lessampler_source *.cpp *.h)

if (MSVC)
    list(APPEND ICON ${PROJECT_BINARY_DIR}/lessampler.rc)
    add_executable(lessampler ${lessampler_source} ${ICON})
else ()
    add_executable(lessampler ${lessampler_source})
endif ()

target_link_libraries(lessampler PRIVATE AudioModel FileIO Dialogs UTAU ConfigUnit Threads::Threads)

if (MSVC)
    target_compile_options(lessampler PUBLIC "/utf-8")
endif ()
