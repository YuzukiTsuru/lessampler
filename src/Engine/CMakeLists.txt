add_library(Engine
        LruCache.h
        RenderEngine.cpp
        RenderEngine.h
        VoicebankModeler.cpp
        VoicebankModeler.h
)
target_link_libraries(Engine PUBLIC
        Core
        Config
        Audio
        Vocoder
        Threads::Threads
)
