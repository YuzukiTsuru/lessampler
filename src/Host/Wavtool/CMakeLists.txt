add_library(Wavtool
        WavAssembly.cpp
        WavAssembly.h
        WavtoolProtocol.cpp
        WavtoolProtocol.h
        WavtoolPlan.cpp
        WavtoolPlan.h
)

target_link_libraries(Wavtool PUBLIC
        Audio
        Core
        tomlplusplus::tomlplusplus
)
