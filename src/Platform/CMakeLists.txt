add_library(Platform
        Dialogs.cpp
        Dialogs.h
)
target_link_libraries(Platform PUBLIC portable_file_dialogs Core)
