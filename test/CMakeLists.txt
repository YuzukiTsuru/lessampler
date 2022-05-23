add_executable(libUTAUTest libUTAUTest.cpp)
target_link_libraries(libUTAUTest UTAU)

add_executable(PitchBendTest PitchBendTest.cpp)
target_link_libraries(PitchBendTest UTAU)

add_executable(ScaleConvertTest ScaleConvertTest.cpp)
target_link_libraries(ScaleConvertTest UTAU)

add_executable(ConfigUnitTest ConfigUnitTest.cpp)
target_link_libraries(ConfigUnitTest ConfigUnit)

add_executable(LOGTest LOGTest.cpp)

add_executable(AudioModelBinTest AudioModelBinTest.cpp)
target_link_libraries(AudioModelBinTest FileIO AudioModel world::tool)

add_executable(WorldAnalysisTest WorldAnalysisTest.cpp)
target_link_libraries(WorldAnalysisTest world::tool world::core)

add_executable(GenerateAudioModelTest GenerateAudioModelTest.cpp ../src/GenerateAudioModel.cpp)
target_link_libraries(GenerateAudioModelTest FileIO AudioModel sndfile)

add_executable(AudioProcessTest AudioProcessTest.cpp)
target_link_libraries(AudioProcessTest AudioProcess AudioModel UTAU world::tool)