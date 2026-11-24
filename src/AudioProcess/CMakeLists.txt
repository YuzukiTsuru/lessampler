add_library(AudioProcess
        AudioProcess.cpp
        AudioProcess.h
        AutoAMP.cpp
        AutoAMP.h
)
target_link_libraries(AudioProcess PRIVATE
        Shine
        AudioModel
        lessampler_common
        Threads::Threads
)
