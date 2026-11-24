add_library(FileIO
        AudioModelIO.cpp
        AudioModelIO.h
        GenerateAudioModel.cpp
        GenerateAudioModel.h
        WavIO.cpp
        WavIO.h
)
target_link_libraries(FileIO PUBLIC
        SndFile::sndfile
        AudioModel
        AudioProcess
        ConfigUnit
        lessampler_common
        Threads::Threads
)
