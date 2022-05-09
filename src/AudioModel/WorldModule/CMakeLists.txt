file(GLOB world_module_source *.cpp *.h)
add_library(WorldModule ${world_module_source})
target_link_libraries(WorldModule world::core)