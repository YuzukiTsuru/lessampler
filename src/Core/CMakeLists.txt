add_library(Core
        MasterPresetKind.h
        SourceFingerprint.cpp
        SourceFingerprint.h
        PreprocessOptions.h
        VoiceStyle.h
)
target_link_libraries(Core PUBLIC lessampler_common)
