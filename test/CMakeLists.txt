add_executable(libUTAUTest libUTAUTest.cpp)
target_link_libraries(libUTAUTest UTAU)

add_executable(PitchBendTest PitchBendTest.cpp)
target_link_libraries(PitchBendTest UTAU)

add_executable(ScaleConvertTest ScaleConvertTest.cpp)
target_link_libraries(ScaleConvertTest UTAU)

add_executable(ConfigUnitTest ConfigUnitTest.cpp)
target_link_libraries(ConfigUnitTest ConfigUnit)

add_executable(LOGTest LOGTest.cpp)

add_executable(VecbinTest VecbinTest.cpp)

add_executable(TimerTest TimerTest.cpp)

add_executable(AudioModelBinTest AudioModelBinTest.cpp)
target_link_libraries(AudioModelBinTest FileIO AudioModel world::tool ConfigUnit)

add_executable(AudioModelBinVersionTest AudioModelBinVersionTest.cpp)
target_link_libraries(AudioModelBinVersionTest FileIO AudioModel world::tool ConfigUnit)

add_executable(WorldAnalysisTest WorldAnalysisTest.cpp)
target_link_libraries(WorldAnalysisTest world::tool world::core)

add_executable(GenerateAudioModelTest GenerateAudioModelTest.cpp)
target_link_libraries(GenerateAudioModelTest FileIO ConfigUnit AudioProcess Threads::Threads AudioModel sndfile)

add_executable(AudioProcessTest AudioProcessTest.cpp)
target_link_libraries(AudioProcessTest AudioProcess AudioModel AudioModelSynthesis Shine world::tool)
