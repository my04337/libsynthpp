cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	Base/Base.cpp
	Base/String.cpp
	
	Threading/TaskSet.cpp

	Debugging/StackTrace.cpp
	Debugging/Logging.cpp
)  
target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
