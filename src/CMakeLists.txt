cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	Base/Base.cpp
	Base/Signal.cpp
	Base/StackTrace.cpp
	Base/Logging.cpp
	
	Threading/EventSignal.cpp
	Threading/TaskDispatcher.cpp

	
	Audio/WavFileOutput.cpp
	Audio/WasapiOutput.cpp
)  
target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
