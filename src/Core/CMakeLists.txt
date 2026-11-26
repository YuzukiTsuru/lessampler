add_library(Core
        MasterPresetKind.h
        SourceFingerprint.cpp
        SourceFingerprint.h
)
target_link_libraries(Core PUBLIC lessampler_common)
