add_subdirectory(Binding)

add_library(Shine
        Shine.cpp
        Shine.h
        ShinePara.h
)
target_link_libraries(Shine PUBLIC UTAU lessampler_common)
