add_library(Audio
        AutoAMP.cpp
        AutoAMP.h
        Envelope.cpp
        Envelope.h
        SampleSource.cpp
        SampleSource.h
        WavIO.cpp
        WavIO.h
)
target_link_libraries(Audio PUBLIC
        Core
        SndFile::sndfile
)
