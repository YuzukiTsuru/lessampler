# Library FileIO
file(GLOB FileIO_source *.cpp *.h)
add_library(FileIO ${FileIO_source})
target_link_libraries(FileIO rapidjson)