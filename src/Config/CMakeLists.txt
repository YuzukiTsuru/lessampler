add_library(Config
        Config.cpp
        Config.h
        ConfigLoader.cpp
        ConfigLoader.h
        ConfigParser.cpp
        ConfigParser.h
        VoicebankConfig.cpp
        VoicebankConfig.h
)
target_link_libraries(Config PUBLIC
        Core
        tomlplusplus::tomlplusplus
)
