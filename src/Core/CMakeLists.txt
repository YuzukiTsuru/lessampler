add_library(Core
        SourceFingerprint.cpp
        SourceFingerprint.h
)
target_link_libraries(Core PUBLIC lessampler_common)
