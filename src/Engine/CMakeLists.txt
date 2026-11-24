add_library(Engine
        VoicebankModeler.cpp
        VoicebankModeler.h
)
target_link_libraries(Engine PUBLIC
        Core
        Config
        Audio
        Analysis
        Backend
        Threads::Threads
)
