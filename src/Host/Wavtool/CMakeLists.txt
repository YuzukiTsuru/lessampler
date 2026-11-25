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

add_executable(lesswavtool
        main.cpp
)

target_link_libraries(lesswavtool PRIVATE Wavtool)

set_target_properties(lesswavtool PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)

install(TARGETS lesswavtool RUNTIME DESTINATION .)

if (APPLE)
    set_target_properties(lesswavtool PROPERTIES INSTALL_RPATH "@loader_path")
elseif (UNIX)
    set_target_properties(lesswavtool PROPERTIES INSTALL_RPATH "$ORIGIN")
endif ()
