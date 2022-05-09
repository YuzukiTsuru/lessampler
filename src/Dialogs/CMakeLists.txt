# Library Dialog
file(GLOB Dialogs_source *.cpp *.h)
add_library(Dialogs ${Dialogs_source})
target_link_libraries(Dialogs)