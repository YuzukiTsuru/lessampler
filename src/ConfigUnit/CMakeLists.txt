# Library ConfigUnit
file(GLOB ConfigUnit_source *.cpp *.h)
add_library(ConfigUnit ${ConfigUnit_source})
target_link_libraries(ConfigUnit PRIVATE inicpp)
