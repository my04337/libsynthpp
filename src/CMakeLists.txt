cmake_minimum_required(VERSION 3.2)

add_library(
	libsynthpp STATIC

	Base/base.cpp
)  
target_include_directories(
             libsynthpp
             PUBLIC  ../include
            )
