add_library(Dialogs
        Dialogs.cpp
        Dialogs.h
)
target_link_libraries(Dialogs PUBLIC portable_file_dialogs lessampler_common)
