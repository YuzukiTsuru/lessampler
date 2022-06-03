# Library UTAU
file(GLOB utau_source *.cpp *.h)
add_library(UTAU ${utau_source})

if (MSVC)
    set_target_properties(UTAU PROPERTIES COMPILE_FLAGS "/wd4819")
endif ()