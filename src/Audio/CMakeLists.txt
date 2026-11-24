add_library(Audio
        WavIO.cpp
        WavIO.h
)
target_link_libraries(Audio PUBLIC
        Core
        SndFile::sndfile
)
