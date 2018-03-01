cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	Base/Base.cpp
	Base/String.cpp
	Base/Signal.cpp
	
	Debugging/StackTrace.cpp
	Debugging/Logging.cpp

	Util/BiquadraticFilter.cpp
)  
target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
