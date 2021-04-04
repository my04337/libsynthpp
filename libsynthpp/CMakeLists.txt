cmake_minimum_required(VERSION 3.18)

add_library(
	libsynthpp STATIC

	src/LSP/Base/Base.cpp
	src/LSP/Base/StackTrace.cpp
	src/LSP/Base/Logging.cpp
	src/LSP/Base/Message.cpp
	
	src/LSP/Threading/Thread.cpp
	src/LSP/Threading/EventSignal.cpp
	src/LSP/Threading/TaskDispatcher.cpp
		
	src/LSP/Audio/WavFileOutput.cpp
	src/LSP/Audio/WasapiOutput.cpp
	src/LSP/Audio/SDLOutput.cpp
	
	src/LSP/MIDI/Parser.cpp
	src/LSP/MIDI/Sequencer.cpp

	src/LSP/Synth/VoiceMapper.cpp
	src/LSP/Synth/Luath.cpp
	
	src/LSP/static_test.cpp)  

target_include_directories(
             libsynthpp
             PUBLIC  src
            )
			
target_link_libraries(libsynthpp SDL2.lib)
add_dependencies(libsynthpp SDL2)