add_library(WorldModule
        F0Estimator.cpp
        F0Estimator.h
        WorldModule.cpp
        WorldModule.h
        WorldPara.h
)
target_link_libraries(WorldModule PUBLIC world::core lessampler_common)
