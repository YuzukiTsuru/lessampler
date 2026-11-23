add_library(WorldModule
        WorldModule.cpp
        WorldModule.h
        WorldPara.h
)
target_link_libraries(WorldModule PUBLIC world::core lessampler_common)
