cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	Base/Base.cpp
	Base/Signal.cpp
	
	Debugging/Stacktrace.cpp

	Util/BiquadraticFilter.cpp
)  
target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
