cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	src/Base/Base.cpp
	src/Base/StackTrace.cpp
	src/Base/Logging.cpp
	src/Base/Message.cpp
	
	src/Threading/Thread.cpp
	src/Threading/EventSignal.cpp
	src/Threading/TaskDispatcher.cpp
		
	src/Audio/WavFileOutput.cpp
	src/Audio/WasapiOutput.cpp
	src/Audio/SDLOutput.cpp
	
	src/MIDI/Parser.cpp
	src/MIDI/Sequencer.cpp
	src/MIDI/Synthesizer/VoiceMapper.cpp
	

	src/static_test.cpp "include/LSP/MIDI/Synthesizer/Voice.hpp" "include/LSP/MIDI/Synthesizer/SimpleVoice.hpp")  

target_include_directories(
             libsynthpp
             PUBLIC  include
            )
			
target_link_libraries(libsynthpp SDL2.lib)
add_dependencies(libsynthpp SDL2)