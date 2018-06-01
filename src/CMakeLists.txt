cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	Base/Base.cpp
	Base/StackTrace.cpp
	Base/Logging.cpp
	Base/Message.cpp
	
	Threading/Thread.cpp
	Threading/EventSignal.cpp
	Threading/TaskDispatcher.cpp
		
	Audio/WavFileOutput.cpp
	Audio/WasapiOutput.cpp
	Audio/SDLOutput.cpp
	
	MIDI/Parser.cpp
	MIDI/Sequencer.cpp
	MIDI/Controller.cpp

	static_test.cpp
)  

target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
			
target_link_libraries(libsynthpp SDL2.lib)
add_dependencies(libsynthpp SDL2)