add_subdirectory(Binding)

file(GLOB shine_source *.cpp *.h)
add_library(Shine ${shine_source})
target_link_libraries(Shine UTAU)