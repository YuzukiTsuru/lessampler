add_subdirectory(libUTAU)

add_library(Shine
        Shine.cpp
        Shine.h
)
target_link_libraries(Shine PUBLIC
        UTAU
        Core
)
