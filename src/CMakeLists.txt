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
	
	MIDI/Messages/Messages.cpp
	MIDI/SMF/Parser.cpp
	MIDI/SMF/Sequencer.cpp

	static_test.cpp
)  
target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
