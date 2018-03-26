cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	Base/Base.cpp
	Base/String.cpp
	Base/Signal.cpp
	
	Threading/EventSignal.cpp
	Threading/TaskDispatcher.cpp

	Debugging/StackTrace.cpp
	Debugging/Logging.cpp
	
	Audio/WavFileOutput.cpp
	Audio/WasapiOutput.cpp
)  
target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
