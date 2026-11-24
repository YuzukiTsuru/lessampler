add_library(ConfigUnit
        ConfigUnit.cpp
        ConfigUnit.h
        ConfigVoiceBank.cpp
        ConfigVoiceBank.h
        ConfigParser.cpp
        ConfigParser.h
        lessConfigure.cpp
        lessConfigure.h
)
target_link_libraries(ConfigUnit PUBLIC tomlplusplus::tomlplusplus lessampler_common)
